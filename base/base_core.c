u32 safe_u64_to_u32(u64 value) {
    debug_assert(value <= max_u32);
    u32 result = (u32)value;
    return result;
}

char *shift_args(char ***argv, int *argc) {
    debug_assert(*argc > 0);
    char *result = **argv;
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}