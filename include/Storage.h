#ifndef STORAGE_H
#define STORAGE_H

#include <SD.h>
#include <vector>
#include <Arduino.h>

#define DEV_LOGS 1

#define CS_PIN 5

namespace Storage
{
    File root;

    void printDirectory(File dir, int numTabs = 0)
    {
        //? while true no parece hacer nada, testear luego.
        while (true)
        {
            File entry = dir.openNextFile();
            if (!entry)
            {
                if (numTabs == 0)
                    Serial.println("** Done **");
                return;
            }

            for (uint8_t i = 0; i < numTabs; i++)
                Serial.print('\t');

            Serial.print(entry.name());

            if (entry.isDirectory())
            {
                Serial.println("/");
                printDirectory(entry, numTabs + 1);
            }
            else
            {
                Serial.print("\t\t");
                Serial.println(entry.size(), DEC);
            }

            entry.close();
        }
    }

    void Init()
    {
        if (!SD.begin(CS_PIN))
        {
#if DEV_LOGS
            Serial.print("SD Card module NOT working!");
#endif
            return;
        }

        root = SD.open("/");

#if DEV_LOGS
        Serial.print("SD Card module working!");
        printDirectory(root);
#endif
    }

    std::vector<String> getFilesName()
    {
        std::vector<String> names;
        File file = root.openNextFile();
        while(true){
            if (!file) break;
            file = file.openNextFile();
            names.push_back(file.name());
        }
        return names;
    }
};
#endif