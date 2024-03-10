#include <playerctl/playerctl.h>
#include <gio/gio.h>
#include <glib.h>

typedef struct {
    char *obj_name;
    void (*func)(PlayerctlPlayer *player, GError **error);
} PlayerCommand;

const PlayerCommand player_cmds[] = {
    {"can-play", &playerctl_player_play_pause},
    {"can-go-next", &playerctl_player_next},
    {"can-go-previous", &playerctl_player_previous},
};

const guint TIMEOUT = 250 * 1000;

static gboolean player_control(PlayerctlPlayer *player, PlayerCommand cmd) {
    GError *error = NULL;

    gboolean can_play = FALSE;
    g_object_get(player, cmd.obj_name, &can_play, NULL);

    if (!can_play) {
        return FALSE;
    }

    cmd.func(player, &error);
    return !error;
}

void mpris_control(char keycode) {
    GError *error = NULL;
    GList *players = NULL;
    PlayerctlPlayerManager *manager = playerctl_player_manager_new(&error);

    g_object_get(manager, "player-names", &players, NULL);

    GList *l = NULL;
    for (l = players; l != NULL; l = l->next) {
        PlayerctlPlayerName *name = l->data;

        PlayerctlPlayer *player = playerctl_player_new_from_name(name, &error);
        if (error != NULL) {
            g_object_unref(player);
            continue;
        }

        gboolean result = FALSE;
        result = player_control(player, player_cmds[keycode]);
        if (result) {
            break;
        }

        g_object_unref(player);
    }
}

gboolean g_date_time_diff_within(GDateTime *end, GDateTime *begin, GTimeSpan limit) {
    GTimeSpan diff = g_date_time_difference(end, begin);
    return diff < limit;
}

guchar g_file_getc(GFile *file) {
    guchar c;
    GFileInputStream *input_stream = g_file_read(file, NULL, NULL);
    g_input_stream_read(
        G_INPUT_STREAM(input_stream),
        &c,
        1,
        NULL,
        NULL
    );
    g_input_stream_close(G_INPUT_STREAM(input_stream), NULL, NULL);
    g_object_unref(input_stream);
    return c;
}

void g_file_putc(GFile *file, guchar c) {
    GFileOutputStream *output_stream = g_file_replace(
        file,
        NULL,
        FALSE,
        G_FILE_CREATE_NONE,
        NULL,
        NULL
    );
    g_output_stream_write(
        G_OUTPUT_STREAM(output_stream),
        &c,
        1,
        NULL,
        NULL
    );
    g_output_stream_close(G_OUTPUT_STREAM(output_stream), NULL, NULL);
    g_object_unref(output_stream);
}

GDateTime *g_file_get_mtime(GFile *file) {
    GError *error = NULL;
    GFileInfo *file_info = g_file_query_info(
        file,
        G_FILE_ATTRIBUTE_TIME_MODIFIED "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
        G_FILE_QUERY_INFO_NONE,
        NULL,
        &error
    );
    if (error != NULL) {
        return NULL;
    }
    GDateTime *mtime = g_file_info_get_modification_date_time(file_info);
    g_object_unref(file_info);
    return mtime;
}

int main() {
    GError *error = NULL;
    guchar state = 0;
    GFile *file = g_file_new_for_path(g_build_path(G_DIR_SEPARATOR_S, g_get_tmp_dir(), "mediakey-state", NULL));
    GDateTime *last_modified_time = g_file_get_mtime(file);
    GDateTime *current_time = g_date_time_new_now_local();

    if (last_modified_time != NULL && g_date_time_diff_within(current_time, last_modified_time, TIMEOUT)) {
        state = g_file_getc(file);
        if (state >= 2) {
            return 0;
        }
        state++;
    }

    g_file_putc(file, state);
    g_usleep(TIMEOUT);

    guchar new_state = g_file_getc(file);
    g_object_unref(file);
    if (new_state == state) {
        mpris_control(state);
    }
    return 0;
}
