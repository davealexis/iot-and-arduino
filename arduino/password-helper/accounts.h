
struct Account {
    byte id[4];
    char *title;
    char *password;
};

Account accounts[] = {
    {
        { 0x00, 0x00, 0x00, 0x00 },
        "Key Fob",
        "<REPLACE WITH PASSWORD>"
    },
    {
        { 0x3A, 0xDB, 0x9F, 0x1A},
        "Key 2",
        "my password too"
    }
};