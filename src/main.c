#include "window.h"
#include "opengl.h"
#include "color.h"
#include "time.h"
#include "types.h"
#include "utils.h"

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <poll.h>
#include <sys/inotify.h>
#endif

void resize_callback(){
    opengl_set_viewport(0, 0, current_window_size.width, current_window_size.height);
}

void keyboard_callback(u32 keycode, KeyState key_state){
    if(keycode == KEY_ESCAPE && key_state == PRESSED){
        window_close();
    }
    if(keycode == KEY_F11 && key_state == PRESSED){
        if(current_window_state == WINDOWED){
            window_set_state(WINDOWED_FULLSCREEN);
        }
        else{
            window_set_state(WINDOWED);
        }
    }
}

#ifdef _WIN32

typedef struct FileWatch{
    HANDLE notification_handle;
    HANDLE directory_handle;
    const char* filepath;
} FileWatch;

#elif __linux__

typedef struct FileWatch{
    i32 file_descriptor;
    struct pollfd* file_descriptors_set;
} FileWatch;

#endif

FileWatch set_file_watch(const char* filepath){
#ifdef _WIN32
    char current_directory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, current_directory);

    wchar_t current_directory_w[MAX_PATH] = {0};
    MultiByteToWideChar(0, 0, current_directory, MAX_PATH, current_directory_w, MAX_PATH);

    HANDLE notification_handle = FindFirstChangeNotificationW(current_directory_w, 0,
        FILE_NOTIFY_CHANGE_LAST_WRITE);

    HANDLE directory_handle = CreateFile(current_directory, FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    return (FileWatch){
        notification_handle,
        directory_handle,
        filepath
    };
#elif __linux__
    i32 file_descriptor = inotify_init1(IN_NONBLOCK);
    i32 watch_descriptor = inotify_add_watch(file_descriptor, filepath, IN_MODIFY);
    struct pollfd file_descriptors_set[1];
    file_descriptors_set[0].fd = file_descriptor;
    file_descriptors_set[0].events = POLLIN;
    return (FileWatch){
        file_descriptor,
        file_descriptors_set
    };
#endif
}

u8 check_file_modified(FileWatch file_watch){
#ifdef _WIN32
    DWORD wait = WaitForSingleObject(file_watch.notification_handle, 0);
    if(wait == WAIT_OBJECT_0){
        DWORD cbBytes;
        FILE_NOTIFY_INFORMATION information_buffer[1024];
        ReadDirectoryChangesW(file_watch.directory_handle, &information_buffer,
            sizeof(information_buffer),
            FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &cbBytes, NULL, NULL);


        WCHAR* pointer = information_buffer->FileName;
        char* filename = malloc(sizeof(char)*(1+information_buffer->FileNameLength/sizeof(WCHAR)));
        i32 c = 0;
        for(u32 i = 0; i < information_buffer->FileNameLength; i+=sizeof(WCHAR)){
            filename[c] = *pointer;
            c++;
            pointer++;
        }
        filename[c] = '\0';
        FindNextChangeNotification(file_watch.notification_handle);

        if(strcmp(filename, file_watch.filepath) == 0){
            return 1;
        }
    }
    return 0;
#elif __linux__
    i32 rc = poll(file_watch.file_descriptors_set, 1, 0);
    char buffer[4096];
    while(1){
        i32 len = read(file_watch.file_descriptor, buffer, sizeof(buffer));
        if(len < 0){
            return 0;
        }
        return 1;
    }
    return 0;
#endif
}

u32 uniforms_locations[3] = {0};
void reload_uniforms_locations(u32 program){
    uniforms_locations[0] = glGetUniformLocation(program, "time");
    uniforms_locations[1] = glGetUniformLocation(program, "resolution");
    uniforms_locations[2] = glGetUniformLocation(program, "mouse");
}

i32 main(){
    u8 err = window_create("GLSL Viewer", 800, 600);
    if(err){
        return 1;
    }
    err = opengl_init(3, 3, 2);
    if(err){
        return 1;
    }

    opengl_set_swap_interval(1);

    window_set_resize_callback(resize_callback);
    window_set_keyboard_callback(keyboard_callback);

    opengl_set_viewport(0, 0, current_window_size.width, current_window_size.height);

    f32 vertices[] = {
        -1,  1, 0,
         1,  1, 0,
         1, -1, 0,
        -1, -1, 0
    };

    u32 indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    u32 vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(f32), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const char* shader_filename = "shader.glsl";

    u32 program = opengl_load_shader("vertex.glsl", shader_filename);
    if(program){
        reload_uniforms_locations(program);
    }

    FileWatch shader_file_watch = set_file_watch(shader_filename);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    u64 start_time = get_time();
    while(1){
        if(!window_event()) break;
        opengl_clear((RGBA){0, 0, 0, 1});

        glBindVertexArray(vao);

        u8 shader_modified = check_file_modified(shader_file_watch);

        if(shader_modified){
            glDeleteShader(program);
            program = opengl_load_shader("vertex.glsl", shader_filename);
            if(program){
                reload_uniforms_locations(program);
            }
        }

        if(program != 0){
            glUseProgram(program);
            glUniform1f(uniforms_locations[0], ((float)get_time()-start_time)/1000000);
            glUniform2f(uniforms_locations[1],
                (float)current_window_size.width,
                (float)current_window_size.height);
            glUniform2f(uniforms_locations[2],
                (float)mouse_position.x,
                (float)mouse_position.y);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        opengl_swap_buffers();
    }

    return 0;
}