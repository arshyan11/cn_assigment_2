#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>   // Include for stat function
#include <libgen.h>     // Include for dirname function

#define MAX_BUFFER_SIZE 1024
#define PORT 8080

GtkWidget *progress_bar;

void send_file(const char *file_path, const char *server_ip) {
    int sock;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read, total_bytes_sent = 0;
    struct stat file_stat;
    off_t total_size;

    // Get the size of the file
    if (stat(file_path, &file_stat) != 0) {
        perror("Error getting file size");
        return;
    }
    total_size = file_stat.st_size;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return;
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    // Open the video file
    file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        close(sock);
        return;
    }

    // Read and send the file in chunks
    while ((bytes_read = fread(buffer, 1, MAX_BUFFER_SIZE, file)) > 0) {
        if (send(sock, buffer, bytes_read, 0) < 0) {
            perror("Send failed");
            break;
        }
        total_bytes_sent += bytes_read;

        // Update progress bar
        float progress = (float)total_bytes_sent / total_size;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress_bar));
    }

    // Send end-of-file signal (optional, based on your protocol)
    // Here we send an empty message to indicate EOF
    send(sock, "", 0, 0);

    // Clean up
    fclose(file);
    close(sock);

    // Create a writable copy of the file_path for dirname
    char *file_path_copy = strdup(file_path);  // Allocate memory for the copy
    if (file_path_copy == NULL) {
        perror("Failed to copy file path");
        return;
    }

    // Open the target folder (assuming a Linux environment)
    char command[1024];
    snprintf(command, sizeof(command), "xdg-open %s", dirname(file_path_copy));  // Use writable copy
    system(command);

    // Free the allocated memory
    free(file_path_copy);
}


static void on_file_selected(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data);  // The file chooser dialog is passed as the user data
    gint response;
    
    // Run the file chooser dialog and get the response
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        // Get the selected file path from the file chooser dialog
        const char *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file_path) {
            send_file(file_path, "127.0.0.1");  // Replace with the actual server IP address
        }
    }
    
    // Close the dialog after the selection
    gtk_widget_destroy(dialog);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window, *button, *file_chooser, *box;

    // Create the application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Video File Sender");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    // Create a vertical box to hold the button and progress bar
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  // Vertical layout with spacing of 10px

    // Create a file chooser dialog (this is different from the GtkButton)
    file_chooser = gtk_file_chooser_dialog_new("Select Video File",
                                               GTK_WINDOW(window),
                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               "_Open", GTK_RESPONSE_ACCEPT,
                                               NULL);

    // Create a button for selecting a file
    button = gtk_button_new_with_label("Select Video File");
    g_signal_connect(button, "clicked", G_CALLBACK(on_file_selected), file_chooser);  // Pass the file chooser dialog as user data

    // Create a progress bar
    progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);

    // Pack the button and progress bar into the box container
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);  // Add button to box
    gtk_box_pack_start(GTK_BOX(box), progress_bar, TRUE, TRUE, 0);  // Add progress bar to box

    // Add the box container to the window
    gtk_container_add(GTK_CONTAINER(window), box);

    // Show all widgets in the window
    gtk_widget_show_all(window);
}






int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.sender", G_APPLICATION_DEFAULT_FLAGS);  // Updated flag
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
