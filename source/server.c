/*
 * Copyright (C) 2023  linas <linas@justforfin.cn>
 * Author: linas <linas@justforfin.cn>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stddef.h>

#include <pen_http/pen_http_server.h>

const char *g_unix_domain = NULL;
const char *g_local_host = NULL;
unsigned short g_local_port = 0;
const char *root_dir = NULL;
const char *secret_key = NULL;
static size_t secret_len = 0;
static size_t root_len = 0;
static char path[256];

static struct {
    pen_http_server_t server_;
} g_self;

PEN_HTTP_PUT(upload) {
    pen_string_t version;
    pen_string_t secret;
    pen_string_t name;
    size_t len = root_len;

    if (PEN_UNLIKELY(!pen_http_req_params(eb, "version", 7, &version)))
        goto error;

    if (PEN_UNLIKELY(!pen_http_req_params(eb, "secret", 6, &secret)))
        goto error;

    if (PEN_UNLIKELY(!pen_http_req_params(eb, "name", 4, &name)))
        goto error;

    if (PEN_UNLIKELY(version.len_ > 8 || name.len_ > 64 || name.len_ == 0 || version.len_ == 0))
        goto error;

    if (secret_len != secret.len_ || strncmp(secret_key, secret.str_, secret_len) != 0)
        goto error;

    len = pen_path_join(path, len, version.str_, version.len_);
    if (access(path, F_OK) == 0) {
        if (access(path, W_OK | X_OK) != 0)
            goto error;
    } else {
        if (mkdir(path, 0755) != 0)
            goto error;
    }
    len = pen_path_join(path, len, name.str_, name.len_);

    return pen_http_save_file(eb, path);
error:
    if (pen_http_resp_code(eb, 404) != 1)
        PEN_WARN("[http] send resp stoped");
    return false;
}

bool
pen_server_init(pen_event_t ev)
{
    if (secret_key == NULL || root_dir == NULL) {
        PEN_ERROR("wrong secret, root dir");
        return false;
    }
    secret_len = strlen(secret_key);
    if (secret_len == 0) {
        PEN_ERROR("wrong secret");
        return false;
    }

    root_len = strlen(root_dir);
    if (root_len == 0 || root_len > 128) {
        PEN_ERROR("wrong root dir");
        return false;
    }

    if (access(root_dir, W_OK | X_OK) != 0) {
        PEN_ERROR("can't access %s", root_dir);
        return false;
    }

    strncpy(path, root_dir, root_len);
    g_self.server_ = pen_http_server_init(ev, g_local_host, g_local_port, g_unix_domain, 5);
    if (PEN_UNLIKELY(g_self.server_ == NULL))
        return false;

    PEN_HTTP_PUT_REGISTER(g_self.server_, /api/file, upload);
    return true;
}

void
pen_server_destroy(void)
{
    pen_http_server_destroy(g_self.server_);
}

