
ui_handle_t createUi(message_cb, void *Master, void *exit)
{
    return NULL;
}
void destroyUi(ui_handle_t)
{
}
void raiseUi(ui_handle_t, const char *)
{
}
void raiseUi(ui_handle_t, const char *, const char *, ...)
{
}
void tickUi(ui_handle_t)
{
    usleep(100000);
}
