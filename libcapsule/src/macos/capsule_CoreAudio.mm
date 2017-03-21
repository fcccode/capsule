
#include "capsule.h"
#include "capsule_macos.h"
#include "playthrough/CAPlayThrough.h"

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>

static CAPlayThroughHost *playThroughHost;
static bool settingUpPlayThrough = false;

static const AudioObjectPropertyAddress devlist_address = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
};

OSStatus capsule_AudioObjectGetPropertyData (AudioObjectID objectID, const AudioObjectPropertyAddress *inAddress, UInt32 inQualifierDataSize, const void *inQualifierData, UInt32 *ioDataSize, void *outData) {

  OSStatus status = AudioObjectGetPropertyData(objectID, inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);

  if (objectID == kAudioObjectSystemObject &&
      inAddress->mElement == devlist_address.mElement &&
      inAddress->mScope == devlist_address.mScope &&
      (inAddress->mSelector == kAudioHardwarePropertyDevices ||
       inAddress->mSelector == kAudioHardwarePropertyDefaultOutputDevice ||
       inAddress->mSelector == kAudioHardwarePropertyDefaultSystemOutputDevice
       )
      ) {

    AudioDeviceID soundflowerId = -1;
    AudioDeviceID builtinId = -1;
    UInt32 size;
    OSStatus nStatus = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &devlist_address, 0, NULL, &size);
    if (nStatus != kAudioHardwareNoError) {
      CapsuleLog("Could not get size of device list");
    }

    AudioDeviceID* devs = (AudioDeviceID*) alloca(size);
    if (!devs) {
      CapsuleLog("Could not allocate device list");
      return status;
    }

    nStatus = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devlist_address, 0, NULL, &size, devs);
    if (nStatus != kAudioHardwareNoError) {
      CapsuleLog("Could not list devices");
    }

    UInt32 numDevs = size / sizeof(AudioDeviceID);

    CapsuleLog("Querying %s, got %d results",
        (inAddress->mSelector == kAudioHardwarePropertyDevices ? "all devices" :
         (inAddress->mSelector == kAudioHardwarePropertyDefaultOutputDevice ? "default output" :
          (inAddress->mSelector == kAudioHardwarePropertyDefaultSystemOutputDevice ? "default system output" : "???"))),
        (unsigned int) numDevs);

    for (UInt32 i = 0; i < numDevs; i++) {
      CFStringRef cfstr = NULL;
      char *ptr = NULL;
      const AudioObjectPropertyAddress nameaddr = {
          kAudioObjectPropertyName,
          kAudioDevicePropertyScopeOutput,
          kAudioObjectPropertyElementMaster
      };

      size = sizeof (CFStringRef);
      AudioDeviceID dev = devs[i];
      AudioObjectGetPropertyData(dev, &nameaddr, 0, NULL, &size, &cfstr);

      if (!cfstr) {
        CapsuleLog("No output name for device %d", (unsigned int) i);
        continue;
      }

      CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr), kCFStringEncodingUTF8);

      ptr = (char *) malloc(len + 1);
      if (!CFStringGetCString(cfstr, ptr, len + 1, kCFStringEncodingUTF8)) {
        CapsuleLog("Could not convert to CString");
      }
      CFRelease(cfstr);

      CapsuleLog("Device #%d: %s", (unsigned int) i, ptr)

      if (strcmp("Soundflower (2ch)", ptr) == 0) {
        CapsuleLog("Found the soundflower!")
        soundflowerId = dev;
      }
      if (strcmp("Built-in Output", ptr) == 0) {
        CapsuleLog("Found the built-in output!")
        builtinId = dev;
      }

      free(ptr);
    }

    if (builtinId != -1 && soundflowerId != -1) {
      numDevs = *ioDataSize / sizeof(AudioDeviceID);
      devs = (AudioDeviceID*) outData;
      CapsuleLog("Pretending default output is soundflower, %d devices to go through", (unsigned int) numDevs);
      for (UInt32 i = 0; i < numDevs; i++) {
        AudioDeviceID dev = devs[i];
        if (dev == builtinId) {
          CapsuleLog("Swapping device %d!\n", (unsigned int) i);
          devs[i] = soundflowerId;
        }
      }

      if (!playThroughHost && !settingUpPlayThrough) {
        settingUpPlayThrough = true;
        playThroughHost = new CAPlayThroughHost(soundflowerId, builtinId);
        if (!playThroughHost) {
          CapsuleLog("ERROR: playThroughHost init failed!");
          return status;
        }
        playThroughHost->Start();
        settingUpPlayThrough = false;
      }
    }
  } else {
    /* CapsuleLog("Called AudioObjectGetPropertyData with unknown parameters"); */
    /* CapsuleLog("mElement is %u instead of %u", inAddress->mElement, devlist_address.mElement); */
    /* CapsuleLog("mScope is %u instead of %u", inAddress->mScope, devlist_address.mScope); */
    /* CapsuleLog("mSelector is %u instead of %u", inAddress->mSelector, devlist_address.mSelector); */
  }

  return status;
}

DYLD_INTERPOSE(capsule_AudioObjectGetPropertyData, AudioObjectGetPropertyData)


