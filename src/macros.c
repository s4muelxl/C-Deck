#include "macros.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <wchar.h>

static char *trim_text(char *text) {
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    if (!*text) return text;
    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) *end-- = '\0';
    return text;
}

static int copy_text(char *destination, size_t capacity, const char *source) {
    size_t length;
    if (!destination || !source || capacity == 0) return 0;
    length = strlen(source);
    if (length >= capacity) return 0;
    memcpy(destination, source, length + 1);
    return 1;
}

static int utf8_text_valid(const char *value, size_t capacity) {
    wchar_t wide[DECK_MACRO_VALUE_MAX];
    size_t length;
    if (!value || capacity == 0) return 0;
    length = strlen(value);
    if (length == 0 || length >= capacity || length >= DECK_MACRO_VALUE_MAX) return 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char ch = (unsigned char)value[i];
        if (ch < 0x20 || ch == 0x7f) return 0;
    }
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1,
                               wide, (int)(sizeof(wide) / sizeof(wide[0])) ) > 0;
}

static int is_allowed_action(const char *value) {
    static const char *allowed[] = {
        "volume_up", "volume_down", "audio_mute", "mic_mute",
        "media_play_pause", "media_previous", "media_next", "sound_devices"
    };
    if (!value) return 0;
    for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i) {
        if (strcmp(value, allowed[i]) == 0) return 1;
    }
    return 0;
}

static int starts_with_ascii_case(const char *value, const char *prefix) {
    while (*prefix) {
        if (!*value || tolower((unsigned char)*value) != tolower((unsigned char)*prefix)) return 0;
        ++value;
        ++prefix;
    }
    return 1;
}

static int contains_ascii_space(const char *value) {
    if (!value) return 1;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        if (*p == ' ' || *p == '\t') return 1;
    }
    return 0;
}

static int is_allowed_open_target(const char *target) {
    const char *extension;
    size_t length;

    if (!utf8_text_valid(target, DECK_MACRO_VALUE_MAX) || strchr(target, '"')) return 0;
    length = strlen(target);

    if (starts_with_ascii_case(target, "https://") ||
        starts_with_ascii_case(target, "http://")) {
        const char *authority = target + (starts_with_ascii_case(target, "https://") ? 8 : 7);
        return authority[0] != '\0' && authority[0] != '/' &&
               authority[0] != '?' && authority[0] != '#' &&
               !contains_ascii_space(target);
    }
    if (starts_with_ascii_case(target, "ms-settings:")) {
        return !contains_ascii_space(target);
    }

    /* A file path is passed as ShellExecuteW's lpFile, never as a command line.
       Spaces in directory/file names are valid; trailing arguments are not. */
    if (length == 0 || target[0] == ' ' || target[length - 1] == ' ') return 0;
    extension = strrchr(target, '.');
    return extension &&
           (_stricmp(extension, ".exe") == 0 || _stricmp(extension, ".lnk") == 0);
}

static int parse_unsigned(const char *value, unsigned int *out_value) {
    char *end = NULL;
    unsigned long parsed;
    if (!value || !out_value || !value[0]) return 0;
    for (const char *p = value; *p; ++p) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    parsed = strtoul(value, &end, 10);
    if (end == value || *end != '\0' || parsed > UINT_MAX) return 0;
    *out_value = (unsigned int)parsed;
    return 1;
}

int macro_step_parse(const char *text, DeckMacroStep *step) {
    if (!text || !step) return 0;
    ZeroMemory(step, sizeof(*step));

    if (strncmp(text, "action:", 7) == 0) {
        step->type = DECK_MACRO_STEP_ACTION;
        if (!copy_text(step->value, sizeof(step->value), text + 7) ||
            !is_allowed_action(step->value)) return 0;
        return 1;
    }
    if (strncmp(text, "open:", 5) == 0) {
        step->type = DECK_MACRO_STEP_OPEN;
        if (!copy_text(step->value, sizeof(step->value), text + 5) ||
            !is_allowed_open_target(step->value)) return 0;
        return 1;
    }
    if (strncmp(text, "delay:", 6) == 0) {
        step->type = DECK_MACRO_STEP_DELAY;
        if (!parse_unsigned(text + 6, &step->delay_ms) ||
            step->delay_ms == 0 || step->delay_ms > DECK_MACRO_STEP_DELAY_MAX_MS) return 0;
        return 1;
    }
    return 0;
}

int macro_validate(const DeckMacro *macro) {
    unsigned int total_delay = 0;
    if (!macro || !macro->configured ||
        !utf8_text_valid(macro->name, sizeof(macro->name)) ||
        macro->step_count == 0 || macro->step_count > DECK_MACRO_STEP_COUNT) return 0;

    for (unsigned int i = 0; i < macro->step_count; ++i) {
        const DeckMacroStep *step = &macro->steps[i];
        if (step->type == DECK_MACRO_STEP_ACTION) {
            if (!is_allowed_action(step->value)) return 0;
        } else if (step->type == DECK_MACRO_STEP_OPEN) {
            if (!is_allowed_open_target(step->value)) return 0;
        } else if (step->type == DECK_MACRO_STEP_DELAY) {
            if (step->delay_ms == 0 || step->delay_ms > DECK_MACRO_STEP_DELAY_MAX_MS ||
                total_delay > DECK_MACRO_TOTAL_DELAY_MAX_MS - step->delay_ms) return 0;
            total_delay += step->delay_ms;
        } else {
            return 0;
        }
    }
    return 1;
}

static int macro_step_is_valid(const DeckMacroStep *step) {
    if (!step || !memchr(step->value, '\0', sizeof(step->value))) return 0;
    if (step->type == DECK_MACRO_STEP_ACTION) return is_allowed_action(step->value);
    if (step->type == DECK_MACRO_STEP_OPEN) return is_allowed_open_target(step->value);
    if (step->type == DECK_MACRO_STEP_DELAY) {
        return step->delay_ms > 0 && step->delay_ms <= DECK_MACRO_STEP_DELAY_MAX_MS;
    }
    return 0;
}

int macro_step_insert(DeckMacro *macro, unsigned int index, const DeckMacroStep *step) {
    if (!macro || !step || macro->step_count > DECK_MACRO_STEP_COUNT ||
        macro->step_count >= DECK_MACRO_STEP_COUNT || index > macro->step_count ||
        !macro_step_is_valid(step)) return 0;
    memmove(&macro->steps[index + 1], &macro->steps[index],
            (macro->step_count - index) * sizeof(macro->steps[0]));
    macro->steps[index] = *step;
    ++macro->step_count;
    return 1;
}

int macro_step_replace(DeckMacro *macro, unsigned int index, const DeckMacroStep *step) {
    if (!macro || !step || macro->step_count == 0 ||
        macro->step_count > DECK_MACRO_STEP_COUNT || index >= macro->step_count ||
        !macro_step_is_valid(step)) return 0;
    macro->steps[index] = *step;
    return 1;
}

int macro_step_remove(DeckMacro *macro, unsigned int index) {
    if (!macro || macro->step_count == 0 ||
        macro->step_count > DECK_MACRO_STEP_COUNT || index >= macro->step_count) return 0;
    if (index + 1 < macro->step_count) {
        memmove(&macro->steps[index], &macro->steps[index + 1],
                (macro->step_count - index - 1) * sizeof(macro->steps[0]));
    }
    --macro->step_count;
    ZeroMemory(&macro->steps[macro->step_count], sizeof(macro->steps[0]));
    return 1;
}

int macro_step_move(DeckMacro *macro, unsigned int from_index, unsigned int to_index) {
    DeckMacroStep moving;
    if (!macro || macro->step_count == 0 ||
        macro->step_count > DECK_MACRO_STEP_COUNT ||
        from_index >= macro->step_count || to_index >= macro->step_count) return 0;
    if (from_index == to_index) return 1;
    moving = macro->steps[from_index];
    if (from_index < to_index) {
        memmove(&macro->steps[from_index], &macro->steps[from_index + 1],
                (to_index - from_index) * sizeof(macro->steps[0]));
    } else {
        memmove(&macro->steps[to_index + 1], &macro->steps[to_index],
                (from_index - to_index) * sizeof(macro->steps[0]));
    }
    macro->steps[to_index] = moving;
    return 1;
}

static int build_macro_path(char *out, size_t capacity, const char *config_path) {
    const char *slash_a;
    const char *slash_b;
    const char *last_slash;
    size_t directory_length;
    int written;
    if (!out || !config_path || !config_path[0] || capacity == 0) return 0;

    slash_a = strrchr(config_path, '/');
    slash_b = strrchr(config_path, '\\');
    last_slash = slash_a;
    if (!last_slash || (slash_b && slash_b > last_slash)) last_slash = slash_b;
    directory_length = last_slash ? (size_t)(last_slash - config_path + 1) : 0;
    written = snprintf(out, capacity, "%.*smacros.cfg", (int)directory_length, config_path);
    return written >= 0 && (size_t)written < capacity;
}

static int utf8_path_to_wide(const char *path, wchar_t *wide, size_t capacity) {
    if (!path || !wide || capacity == 0 || capacity > INT_MAX) return 0;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                               wide, (int)capacity) > 0;
}

static int parse_macro_section(const char *section_name) {
    char *end = NULL;
    long id;
    if (strncmp(section_name, "[macro", 6) != 0) return -1;
    id = strtol(section_name + 6, &end, 10);
    if (end == section_name + 6 || !end || end[0] != ']' || end[1] != '\0' ||
        id < 1 || id > DECK_MACRO_COUNT) return -2;
    return (int)id - 1;
}

int macro_store_load(DeckMacroStore *store, const char *config_path) {
    wchar_t wide_path[DECK_MACRO_PATH_MAX];
    FILE *file;
    char line[1024];
    DeckMacro parsed[DECK_MACRO_COUNT];
    unsigned int seen_steps[DECK_MACRO_COUNT];
    unsigned int declared_steps[DECK_MACRO_COUNT];
    int section = -1;
    int version = 0;

    if (!store || !build_macro_path(store->path, sizeof(store->path), config_path) ||
        !utf8_path_to_wide(store->path, wide_path, DECK_MACRO_PATH_MAX)) return 0;

    ZeroMemory(parsed, sizeof(parsed));
    ZeroMemory(seen_steps, sizeof(seen_steps));
    ZeroMemory(declared_steps, sizeof(declared_steps));
    store->loaded = 0;
    store->read_only = 0;
    ZeroMemory(store->macros, sizeof(store->macros));

    if (GetFileAttributesW(wide_path) == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            store->loaded = 1;
            return 1;
        }
        store->read_only = 1;
        return 0;
    }

    file = _wfopen(wide_path, L"r");
    if (!file) {
        store->read_only = 1;
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        int has_newline = strchr(line, '\n') != NULL;
        char *entry = trim_text(line);
        char *equals;
        if (!has_newline && !feof(file)) {
            fclose(file);
            store->read_only = 1;
            return 0;
        }
        if (!entry[0] || entry[0] == '#' || entry[0] == ';') continue;

        if (entry[0] == '[') {
            if (_stricmp(entry, "[meta]") == 0) section = -2;
            else {
                section = parse_macro_section(entry);
                if (section == -2) {
                    fclose(file);
                    store->read_only = 1;
                    return 0;
                }
            }
            continue;
        }

        equals = strchr(entry, '=');
        if (!equals) continue;
        *equals++ = '\0';
        entry = trim_text(entry);
        equals = trim_text(equals);

        if (section == -2 && _stricmp(entry, "version") == 0) {
            unsigned int parsed_version;
            if (!parse_unsigned(equals, &parsed_version) || parsed_version > INT_MAX) {
                fclose(file);
                store->read_only = 1;
                return 0;
            }
            version = (int)parsed_version;
        } else if (section >= 0) {
            DeckMacro *macro = &parsed[section];
            if (_stricmp(entry, "name") == 0) {
                if (!copy_text(macro->name, sizeof(macro->name), equals)) {
                    fclose(file);
                    store->read_only = 1;
                    return 0;
                }
            } else if (_stricmp(entry, "step_count") == 0) {
                if (!parse_unsigned(equals, &declared_steps[section]) ||
                    declared_steps[section] > DECK_MACRO_STEP_COUNT) {
                    fclose(file);
                    store->read_only = 1;
                    return 0;
                }
            } else if (starts_with_ascii_case(entry, "step")) {
                unsigned int step_id;
                DeckMacroStep step;
                if (!parse_unsigned(entry + 4, &step_id) || step_id == 0 ||
                    step_id > DECK_MACRO_STEP_COUNT ||
                    (seen_steps[section] & (1u << (step_id - 1))) != 0 ||
                    !macro_step_parse(equals, &step)) {
                    fclose(file);
                    store->read_only = 1;
                    return 0;
                }
                macro->steps[step_id - 1] = step;
                seen_steps[section] |= 1u << (step_id - 1);
            }
        }
    }

    if (ferror(file)) {
        fclose(file);
        store->read_only = 1;
        return 0;
    }
    fclose(file);

    if (version != 1) {
        store->read_only = 1;
        return 0;
    }

    for (int i = 0; i < DECK_MACRO_COUNT; ++i) {
        unsigned int expected;
        if (!parsed[i].name[0] && declared_steps[i] == 0 && seen_steps[i] == 0) continue;
        if (!parsed[i].name[0] || declared_steps[i] == 0) {
            store->read_only = 1;
            return 0;
        }
        expected = (1u << declared_steps[i]) - 1u;
        if (seen_steps[i] != expected) {
            store->read_only = 1;
            return 0;
        }
        parsed[i].step_count = declared_steps[i];
        parsed[i].configured = 1;
        if (!macro_validate(&parsed[i])) {
            store->read_only = 1;
            return 0;
        }
    }

    memcpy(store->macros, parsed, sizeof(parsed));
    store->loaded = 1;
    return 1;
}

static int write_macro_step(FILE *file, unsigned int step_id, const DeckMacroStep *step) {
    if (step->type == DECK_MACRO_STEP_ACTION) {
        return fprintf(file, "step%u=action:%s\n", step_id, step->value) >= 0;
    }
    if (step->type == DECK_MACRO_STEP_OPEN) {
        return fprintf(file, "step%u=open:%s\n", step_id, step->value) >= 0;
    }
    if (step->type == DECK_MACRO_STEP_DELAY) {
        return fprintf(file, "step%u=delay:%u\n", step_id, step->delay_ms) >= 0;
    }
    return 0;
}

int macro_store_save(DeckMacroStore *store) {
    wchar_t wide_path[DECK_MACRO_PATH_MAX];
    wchar_t temporary[DECK_MACRO_PATH_MAX];
    wchar_t backup[DECK_MACRO_PATH_MAX];
    FILE *file;
    size_t path_length;
    int written = 1;

    if (!store || !store->loaded || store->read_only ||
        !utf8_path_to_wide(store->path, wide_path, DECK_MACRO_PATH_MAX)) return 0;
    for (int i = 0; i < DECK_MACRO_COUNT; ++i) {
        if (store->macros[i].configured && !macro_validate(&store->macros[i])) return 0;
    }

    path_length = wcslen(wide_path);
    if (path_length + 5 >= DECK_MACRO_PATH_MAX) return 0;
    wcscpy(temporary, wide_path);
    wcscat(temporary, L".tmp");
    wcscpy(backup, wide_path);
    wcscat(backup, L".bak");

    file = _wfopen(temporary, L"w");
    if (!file) return 0;
    if (fprintf(file, "; C-Deck macros. Versioned file; only allowlisted steps are accepted.\n[meta]\nversion=1\n") < 0) {
        written = 0;
    }
    for (int i = 0; written && i < DECK_MACRO_COUNT; ++i) {
        const DeckMacro *macro = &store->macros[i];
        if (!macro->configured) continue;
        if (fprintf(file, "\n[macro%d]\nname=%s\nstep_count=%u\n", i + 1,
                    macro->name, macro->step_count) < 0) {
            written = 0;
            break;
        }
        for (unsigned int j = 0; written && j < macro->step_count; ++j) {
            if (!write_macro_step(file, j + 1, &macro->steps[j])) written = 0;
        }
    }
    if (fflush(file) != 0) written = 0;
    if (fclose(file) != 0) written = 0;
    if (!written) {
        DeleteFileW(temporary);
        return 0;
    }

    if (GetFileAttributesW(wide_path) != INVALID_FILE_ATTRIBUTES &&
        !CopyFileW(wide_path, backup, FALSE)) {
        DeleteFileW(temporary);
        return 0;
    }
    if (!MoveFileExW(temporary, wide_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary);
        return 0;
    }
    return 1;
}

size_t macro_store_count(const DeckMacroStore *store) {
    size_t count = 0;
    if (!store || !store->loaded) return 0;
    for (int i = 0; i < DECK_MACRO_COUNT; ++i) {
        if (store->macros[i].configured) ++count;
    }
    return count;
}

const DeckMacro *macro_store_get(const DeckMacroStore *store, unsigned int macro_id) {
    if (!store || !store->loaded || macro_id == 0 || macro_id > DECK_MACRO_COUNT) return NULL;
    return store->macros[macro_id - 1].configured ? &store->macros[macro_id - 1] : NULL;
}

DeckMacroResult macro_execute(const DeckMacro *macro,
                              DeckMacroActionExecutor action_executor,
                              DeckMacroOpenExecutor open_executor,
                              DeckMacroDelayExecutor delay_executor,
                              DeckMacroCancelCheck cancel_check,
                              void *context) {
    if (!macro || !macro_validate(macro)) return DECK_MACRO_INVALID_DEFINITION;

    for (unsigned int i = 0; i < macro->step_count; ++i) {
        const DeckMacroStep *step = &macro->steps[i];
        if (cancel_check && cancel_check(context)) return DECK_MACRO_CANCELLED;

        if (step->type == DECK_MACRO_STEP_ACTION) {
            if (!action_executor || action_executor(step->value, context) != 0) {
                return DECK_MACRO_EXECUTION_FAILED;
            }
        } else if (step->type == DECK_MACRO_STEP_OPEN) {
            if (!open_executor || open_executor(step->value, context) != 0) {
                return DECK_MACRO_EXECUTION_FAILED;
            }
        } else if (step->type == DECK_MACRO_STEP_DELAY) {
            if (!delay_executor || delay_executor(step->delay_ms, context) != 0) {
                if (cancel_check && cancel_check(context)) return DECK_MACRO_CANCELLED;
                return DECK_MACRO_EXECUTION_FAILED;
            }
        } else {
            return DECK_MACRO_INVALID_DEFINITION;
        }
    }
    return DECK_MACRO_OK;
}
