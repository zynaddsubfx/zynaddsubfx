#pragma once
#include <string>
#include <rtosc/ports.h>

extern const rtosc::Ports preset_ports;
struct Clipboard {
    std::string data;
    std::string type;
};

Clipboard clipboardCopy(class MiddleWare &mw, std::string url);

void presetCopy(std::string url, std::string name);
void presetPaste(std::string url, std::string name);
void presetPaste(std::string url, int);
void presetDelete(int);
void presetRescan();
std::string presetClipboardType();
bool presetCheckClipboardType();

extern MiddleWare *middlewarepointer;
