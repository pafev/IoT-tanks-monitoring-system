#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

const int localId = 1;
const int destIdArraySize = 1;
const int destIdArray[destIdArraySize] = {2};

const unsigned int timeout = 640000;
const unsigned int timeoutPoll = 15000;


void setup() {
    setupSerial();
    setupLoRa();
}

void setupSerial() {
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Serial iniciada com sucesso");
}

void setupLoRa() {
    if (!LoRa.begin(915E6)) {
        Serial.println("Erro ao iniciar o modulo LoRa");
        while(1);
    }
    Serial.println("Modulo LoRa iniciado com sucesso");
}


void sendPacket(int mode, int senderId, int destId, String content="") {
    LoRa.beginPacket();
    LoRa.write(mode);
    LoRa.write(senderId);
    LoRa.write(destId);
    LoRa.write(content.length());
    LoRa.print(content);
    LoRa.endPacket();
    LoRa.receive();
}

void loop() {
    setTimer(8000);

    unsigned long startTimeGateway = millis();
    for (int i = 0; i < destIdArraySize; i++) {

        bool packetSent = false;
        unsigned long startTimeTank = millis();
        while ((millis() - startTimeTank) < timeoutPoll && !packetSent) {
            Serial.println("Fazendo poll para sender de id " + String(destIdArray[i]));
            sendPacket(1, localId, destIdArray[i]);
            packetSent =  waitForData();
        }
    
        Serial.println("Mandando sender de id " + String(destIdArray[i]) + " dormir");
        unsigned long endTimeTank = millis();
        sendPacket(2, localId, destIdArray[i], String(timeout - (endTimeTank - startTimeTank)));
    }

    Serial.println("Dormindo zzzzzzz...");
    unsigned long endTimeGateway = millis();
    setTimer(timeout - (endTimeGateway - startTimeGateway));
    Serial.println("Acordei :)");
}

void setTimer(unsigned int duration) {
  unsigned long startTimer = millis();
  while (millis() - startTimer < duration);
}

bool waitForData() {
    Serial.println("Aguardando pacote com dados");
    while (1) {
        if (LoRa.parsePacket()) {
            int mode = LoRa.read();
            int tankId = LoRa.read();
            int gatewayId = LoRa.read();
            int contentLength = LoRa.read();

            if (gatewayId == localId) {
                String content = "";
                while (LoRa.available()) {
                    content += (char)LoRa.read();
                }

                if (mode != 0 || contentLength != content.length()) {
                    Serial.println("Pacote corrompido recebido");
                    return false;
                }

                int rssiValue = LoRa.packetRssi();
                float snrValue = LoRa.packetSnr();

                Serial.println("# " + String(tankId) + " " + String(gatewayId) + " " + content + " " + String(rssiValue) + " " + String(snrValue));
                return true;
            }

        }
    }
}
