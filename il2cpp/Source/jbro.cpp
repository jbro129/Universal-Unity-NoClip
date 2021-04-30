#include <jni.h>
#include <android/log.h>
#include <Substrate/CydiaSubstrate.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <bits/sysconf.h>
#include <sys/mman.h>

#define LOG_TAG "JbroMain"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


uintptr_t getLibraryBase(const char *libName) {
    uintptr_t retAddr = 0;

    char fileName[255];
    memset(fileName, 0, sizeof(fileName));

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    snprintf(fileName, sizeof(fileName), "/proc/%d/maps", getpid());
    FILE *fp = fopen(fileName, "rt");
    if (fp != NULL) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, libName) != NULL) {
                retAddr = (uintptr_t) strtoul(buffer, NULL, 16);
                break;
            }
        }
        fclose(fp);
    }
    return retAddr;
}

uintptr_t getAbsoluteAddress(const char *libName, uintptr_t relativeAddr) {
    uintptr_t base = getLibraryBase(libName);
    if (base == 0)
        return 0;
    return (base + relativeAddr);
}

uintptr_t location;

/*
__attribute__((constructor))void initializer() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, started, NULL);
}
*/

uintptr_t base(const char *soname)
{
    void *imagehandle = dlopen(soname, RTLD_LOCAL | RTLD_LAZY);
    if (soname == NULL)
        return NULL;
    if (imagehandle == NULL){
        return NULL;
    }
    uintptr_t * irc = NULL;
    FILE *f = NULL;
    char line[200] = {0};
    char *state = NULL;
    char *tok = NULL;
    char * baseAddr = NULL;
    if ((f = fopen("/proc/self/maps", "r")) == NULL)
        return NULL;
    while (fgets(line, 199, f) != NULL)
    {
        tok = strtok_r(line, "-", &state);
        baseAddr = tok;
        tok = strtok_r(NULL, "\t ", &state);
        tok = strtok_r(NULL, "\t ", &state); // "r-xp" field
        tok = strtok_r(NULL, "\t ", &state); // "0000000" field
        tok = strtok_r(NULL, "\t ", &state); // "01:02" field
        tok = strtok_r(NULL, "\t ", &state); // "133224" field
        tok = strtok_r(NULL, "\t ", &state); // path field

        if (tok != NULL) {
            int i;
            for (i = (int)strlen(tok)-1; i >= 0; --i) {
                if (!(tok[i] == ' ' || tok[i] == '\r' || tok[i] == '\n' || tok[i] == '\t'))
                    break;
                tok[i] = 0;
            }
            {
                size_t toklen = strlen(tok);
                size_t solen = strlen(soname);
                if (toklen > 0) {
                    if (toklen >= solen && strcmp(tok + (toklen - solen), soname) == 0) {
                        fclose(f);
                        return (uintptr_t)strtoll(baseAddr,NULL,16);
                    }
                }
            }
        }
    }
    fclose(f);
    return NULL;
}

uintptr_t getRealOffset(uintptr_t offset)
{
    if (location <= 0)
    {
        location = getLibraryBase("libil2cpp.so");
    }
    //LOGD("Offset #1: %i", offset);
    //LOGD("Offset #2: %i", bkase);
    return location + offset;
}
bool togglenoclip = false;
bool noclip = false;

void (*_Update)(void* _this);
void Update(void* _this)
{
    if (_this)
    {
        void* skinName = *(void **)((uint32_t)_this + 0x320); // internal SkinName mySkinName; // 0x320
        void* firstPersonControl = *(void **)((uint32_t)skinName + 0xD8); // public FirstPersonControlSharp firstPersonControl; // 0xD8
        void* characterController = *(void **)((uint32_t)firstPersonControl + 0x74); // internal CharacterController character; // 0x74

        void (*CharacterController_set_radius)(void* character, float radius) = (void (*)(void *, float ))getRealOffset(0xEAB82C); // CharacterController$$set_radius

        if (togglenoclip) // check to see if u have ever enabled no clip
        {	
            if (noclip) // turn on noclip
            {
                CharacterController_set_radius(characterController, INFINITY);
            }
            else // turn off noclip
            {
                CharacterController_set_radius(characterController, 0.35f); // In order to get the original radius, you will have to get the value of the default radius. If a get_radius does no exist, then experiment around to find a good value. For PG3D I figured out that 0.35f is a good value.
            }
        }
        _Update(_this);
}


MSHookFunction((void *) getRealOffset(offset), (void *) &Update, (void **) &_Update); // Player_move_c UpdateEffects   

