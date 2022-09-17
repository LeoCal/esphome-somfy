#include "esphome.h"
using namespace esphome;
#include <SomfyRts.h>
#include <Ticker.h>
#include "FS.h"
#include <LITTLEFS.h>
#define SPIFFS LittleFS

// cmd 11 - program mode
// cmd 16 - porgram mode for grail curtains
// cmd 21 - delete rolling code file
// cmd 41 - List files
// cmd 51 - Test filesystem.
// cmd 61 - Format filesystem and test.
// cmd 71 - Show actual rolling code
// cmd 81 - Get all rolling code
// cmd 85 - Write new rolling codes

#define REMOTE_TX_PIN D0
#define REMOTE_FIRST_ADDR 0x121311   // <- Change remote name and remote code here!
#define REMOTE_COUNT 5   // <- Number of somfy blinds.

typedef enum {
  FILE_REMOTE,
  FILE_POSITION,
} file_type;

int xcode[REMOTE_COUNT];
uint16_t iCode[REMOTE_COUNT];

char const * string2char(String command) {
  if(command.length()!=0){
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
  return "";
}

String filePath(file_type ftype, int remoteId) {
  String path = "/data/";
  if (ftype == FILE_REMOTE)
    path += "remote/";
  else if (ftype == FILE_POSITION)
    path += "pos/";
  path += REMOTE_FIRST_ADDR + remoteId;
  path += ".txt";
  return path;
}

bool fileExists(file_type ftype, int remoteId) {
  bool existing = false;
  SPIFFS.begin();
  String arq = filePath(ftype, remoteId);
  if (SPIFFS.exists(arq))
    existing = true;
  SPIFFS.end();
  return existing;
}

uint16_t readFromFile(file_type ftype, int remoteId) {
  uint16_t val = 0;
  SPIFFS.begin();
  String arq = filePath(ftype, remoteId);
  if (SPIFFS.exists(arq)) {
    File f = SPIFFS.open(arq, "r");
    if (f) {
      String line = f.readStringUntil('\n');
      val = line.toInt();
      f.close();
    } else {
      ESP_LOGW("file", "File open failed");
      val = -1;
    }
  }
  SPIFFS.end();
  return val;
}

void writeToFile(file_type ftype, int remoteId, uint16_t val) {
  SPIFFS.begin();
  String arq = filePath(ftype, remoteId);
  File f = SPIFFS.open(arq, "w");
  if (f) {
    f.println(val);
    f.close();
    ESP_LOGI("file", "Written value: %d", val);
  } else {
    ESP_LOGW("file","File creation failed");
  }
  SPIFFS.end();
}

uint16_t getCodeFromFile(int remoteId) {
  return readFromFile(FILE_REMOTE, remoteId);
}

void writeCodeToFile(int remoteId, uint16_t code) {
  writeToFile(FILE_REMOTE, remoteId, code);
}

uint16_t getPositionFromFile(int remoteId) {
  return readFromFile(FILE_POSITION, remoteId);
}

void writePositionToFile(int remoteId, uint16_t pos) {
  writeToFile(FILE_POSITION, remoteId, pos);
}

void getCodeFromAllFiles() {
  uint16_t code = 0;
  ESP_LOGW("somfy", "Get all rolling codes from files");
  for (int i = 0; i < REMOTE_COUNT; i++) {
    code = getCodeFromFile(i);
    ESP_LOGI("somfy", "Remoteid: %d - Code: %d", REMOTE_FIRST_ADDR + i, code);
    xcode[i] = code;
  }
}


class RFsomfy : public Component, public Cover {

  private:
  int index;
  uint16_t open_time_ms, close_time_ms;
  Ticker ticker;

  public:
  int remoteId = -1;    
  unsigned char frame[7];

  void set_code(const uint16_t code) { 
    iCode[remoteId] = code;
    writeCodeToFile(remoteId, code);
    xcode[remoteId] = code;
  }

  void readFile() {
    ESP_LOGI("info","Reading file");
    File f = SPIFFS.open("/myFile.txt", "r");
    if (!f) {
      ESP_LOGW("info","File not avaliable");
    } else if (f.available()<=0) {
      ESP_LOGW("info","File exist but not avaliable");
    } else {
      ESP_LOGI("info","File read OK");
      String ssidString = f.readStringUntil('#');
    }
    f.close();
  }

  void writeFile() {
    ESP_LOGI("info","Writing file");
    File f = SPIFFS.open("/myFile.txt", "w");
    if (!f) {
      ESP_LOGW("info","File write failed");
    } else {
      ESP_LOGI("info","Write_OK");
      f.print("networkConfig");
      f.print("#");
      f.flush();
      f.close();
    }
  }

  void testFs() {
    ESP_LOGI("FS","Testing filesystem!");
    if (!SPIFFS.begin()) {
      ESP_LOGW("FS","error while mounting filesystem!");
    } else {
      readFile();
      writeFile();
      readFile();
    }
    SPIFFS.end();
  }

  /* Definition of SomfyRts devices */
  SomfyRts rtsDevices[REMOTE_COUNT] = {
    SomfyRts(REMOTE_FIRST_ADDR),
    SomfyRts(REMOTE_FIRST_ADDR + 1),
    SomfyRts(REMOTE_FIRST_ADDR + 2),
    SomfyRts(REMOTE_FIRST_ADDR + 3),
    SomfyRts(REMOTE_FIRST_ADDR + 4)
  };

  RFsomfy(int id, uint16_t open_time, uint16_t close_time) : Cover() { //register
    index = id;
    remoteId = index;
    /* Convert opening and closing time from seconds to msec */
    open_time_ms = open_time * 1000;
    close_time_ms = close_time * 1000;
    ESP_LOGI("somfy", "Cover %d", index);
  }

  void setup() override {
    // This will be called by App.setup() for each instantiated device
    ESP_LOGI("somfy", "Setup device RemoteId:%d Index:%d", REMOTE_FIRST_ADDR + index, index);
    rtsDevices[index].init();
    xcode[index] = index;

    /* Get rolling code from file */
    xcode[index] = getCodeFromFile(index);
    ESP_LOGI("somfy", "Remoteid: %d - Code: %d", REMOTE_FIRST_ADDR + index, xcode[index]);

    /* Initialize position to open (100) in the corresponding file */
    uint16_t ppos = 100;
    ESP_LOGI("somfy", "Writing init position %u to file for RemoteId:%d", ppos, REMOTE_FIRST_ADDR + index);
    writePositionToFile(REMOTE_FIRST_ADDR + index, ppos); 
  }

  // delete rolling code 0...n
  void delete_code(int remoteId) {
    SPIFFS.begin();
    String path = filePath(FILE_REMOTE, remoteId);
    //SPIFFS.remove(path);
    ESP_LOGI("somfy","Deleted remote %i", remoteId);
    SPIFFS.end();
  }


  /* Mandatory method for custom covers in ESPHome */
  CoverTraits get_traits() override {
    auto traits = CoverTraits();
    traits.set_is_assumed_state(true);
    traits.set_supports_position(true);
    traits.set_supports_tilt(true); // to send other commands
    return traits;
  }
  
  
  /* Mandatory method for custom covers in ESPHome
     This will be called every time the user requests a state change */
  void control(const CoverCall &call) override {
    
    ESP_LOGI("somfy", "Using remote %d", REMOTE_FIRST_ADDR + index);
    ESP_LOGI("somfy", "Remoteid %d", remoteId);
    ESP_LOGI("somfy", "index %d", index);
    
    if (call.get_position().has_value()) {
      uint16_t curr_ppos = getPositionFromFile(REMOTE_FIRST_ADDR + index);
      ESP_LOGI("somfy", "current position is: %u", curr_ppos);
      float curr_pos = ((float)curr_ppos)/100;

      // Write pos (range 0-1) to cover
      float pos = *call.get_position();
      uint16_t ppos = pos * 100;
      ESP_LOGI("somfy", "target position is: %u", ppos);

      if (ppos == 0) {
        ESP_LOGI("somfy","POS 0 - Send command Down");
        rtsDevices[remoteId].sendCommandDown();
        pos = 0.00;
      } else if (ppos == 100) {
        ESP_LOGI("somfy","POS 100 - Send command Up");
        rtsDevices[remoteId].sendCommandUp();
        pos = 1.00;
      } else {
        float delta_pos = abs(pos - curr_pos);
        int delta_delay;
        if (ppos > curr_ppos) {
          delta_delay = delta_pos * open_time_ms;
          ESP_LOGI("somfy", "POS %u - Send command Up, wait %.2f seconds and then send command Stop",
                   ppos, (float)delta_delay/1000);
          rtsDevices[remoteId].sendCommandUp();
        } else {
          delta_delay = delta_pos * close_time_ms;
          ESP_LOGI("somfy", "POS %u - Send command Down, wait %.2f seconds and then send command Stop",
                   ppos, (float)delta_delay/1000);
          rtsDevices[remoteId].sendCommandDown();
        }
        delay(delta_delay);
        rtsDevices[remoteId].sendCommandStop();
      }

      // Publish new state
      this->position = pos;
      this->publish_state();

      // Write new position to the file
      ESP_LOGI("somfy", "Writing new position %u to file", ppos);
      writePositionToFile(REMOTE_FIRST_ADDR + index, ppos);
    }

    if (call.get_stop()) {
      // User requested cover stop
      ESP_LOGI("somfy","get_stop remote ID=%d - Send command Stop", remoteId);
      rtsDevices[remoteId].sendCommandStop();
    }
    
    if (call.get_tilt().has_value()) {
      auto tpos = *call.get_tilt();
      int xpos = tpos * 100;
      ESP_LOGI("tilt", "Command tilt xpos: %d", xpos);

      if (xpos == 11) {
        ESP_LOGI("tilt","program mode");
        rtsDevices[remoteId].sendCommandProg();
        delay(1000);
      }
      if (xpos == 16) {
        ESP_LOGI("tilt","program mode - grail");
        rtsDevices[remoteId].sendCommandProgGrail();
        delay(1000);
      }
      if (xpos == 21) {
        ESP_LOGI("tilt","delete file");
        delete_code(remoteId);
        delay(1000);
      }
      
      if (xpos == 41) {
        ESP_LOGI("tilt","List Files");
        String str = "";
        SPIFFS.begin();
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
          str += dir.fileName();
          str += " / ";
          str += dir.fileSize();
          str += "\r\n";
        }
        //ESP_LOGI("files", string2char(str));
        SPIFFS.end();
      }
      
      if (xpos == 51) {
        ESP_LOGI("tilt","51 mode");
        testFs();
      }
      
      if (xpos == 61) {
        ESP_LOGI("tilt","61 mode");
        SPIFFS.begin();

        if (!SPIFFS.exists("/formatComplete.txt")) {
          ESP_LOGW("file", "Please wait 30 s for FS to be formatted");
          SPIFFS.format();
          delay(30000);
          ESP_LOGW("file", "Spiffs formatted");
        
          File f = SPIFFS.open("/formatComplete.txt", "w");
          if (!f) {
            ESP_LOGW("file", "file open failed");
          } else {
            f.println("Format Complete");
            ESP_LOGW("file", "Format Complete");
          } 
        } else {
          ESP_LOGW("file", "SPIFFS is formatted. Moving along...");
        }
        SPIFFS.end();
      }
     
      if (xpos == 71) {
        // get rolling code from file
        uint16_t code = 0;
        code = getCodeFromFile(remoteId);
        ESP_LOGI("file", "Code: %d", code);
        xcode[remoteId] = code;
      }

      if (xpos == 81) {
        // get all rolling code from file
        getCodeFromAllFiles();
      }

      if (xpos == 85) {
        // Write new rolling codes
        for (int i=0; i<REMOTE_COUNT; i++) {
          writeCodeToFile(REMOTE_FIRST_ADDR + i, iCode[i]);
        }
      }

      /* Don't publish
      this->tilt = tpos;
      this->publish_state();
      */ 
    }
    delay(50);
  }
};

class RFsomfyInfo : public PollingComponent, public TextSensor {
  public:
  RFsomfyInfo() : PollingComponent(15000) {}

  // This will be called by App.setup()
  void setup() override {
  }

  // This will be called every "update_interval" milliseconds.
  void update() override {
    // Publish state
    char tmp[REMOTE_COUNT * 100];
    strcpy (tmp,"");
    char str[5];
    char str2[3];
    String rem;
    boolean bl_code {false};
    for (int i=0; i<REMOTE_COUNT; i++) {
      String rem = String(REMOTE_FIRST_ADDR + i, HEX);
      char line[100];
      sprintf(line, "\n ( %u - #%s) - code: %d / ", i, string2char(rem), xcode[i]);
      strcat(tmp, line);
      if (iCode[i] != 0) {
        bl_code = true;
        ESP_LOGI("icode", "%u : icode: %d", i, iCode[i]);
      }
    }
    publish_state(tmp);
    if (bl_code) {
      ESP_LOGW("set_code", "Atention! After set code, and it works, remove it from your YAML");
    }
  }
};
