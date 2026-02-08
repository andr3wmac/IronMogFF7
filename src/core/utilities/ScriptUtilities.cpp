#include "ScriptUtilities.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#include <string>

void ScriptUtilities::decompileWorldScript(GameManager* game, uintptr_t startAddress, size_t sizeInBytes)
{
    uint8_t* data = new uint8_t[sizeInBytes];
    game->read(startAddress, sizeInBytes, data);

    uint16_t* opCodes = (uint16_t*)data;
    size_t opCodeCount = sizeInBytes / 2;

    int line = 0;
    for (int i = 0; i < opCodeCount; ++i)
    {
        std::string opText = "";

        if (i == 91)
        {
            LOG("-- END OF NORMAL SCRIPT --");
        }

        int opCodeOffset = (i * 2);

        uint16_t opCode = opCodes[i];
        if (opCode == 0x017) { opText = "LOGICAL NOT"; }
        if (opCode == 0x070) { opText = "EQUAL"; }
        if (opCode == 0x0c0) { opText = "LOGICAL OR"; }
        if (opCode == 0x0e0) { opText = "WRITE"; }
        if (opCode == 0x100) { opText = "RESET"; }
        if (opCode == 0x110) { opText = "PUSH CONSTANT " + std::to_string(opCodes[++i]); }
        if (opCode == 0x114) { opText = "PUSH SAVEMAP BIT " + std::to_string(opCodes[++i]); }
        if (opCode == 0x118) { opText = "PUSH SAVEMAP BYTE " + std::to_string(opCodes[++i]); }
        if (opCode == 0x119) { opText = "PUSH BYTE FROM BANK1 " + std::to_string(opCodes[++i]); }
        if (opCode == 0x11b) { opText = "PUSH SPECIAL " + std::to_string(opCodes[++i]); }
        if (opCode == 0x200) { opText = "GOTO " + std::to_string(opCodes[++i]); }
        if (opCode == 0x201) { opText = "GOTO IF FALSE " + std::to_string(opCodes[++i]); }
        if (opCode == 0x203) { opText = "RETURN"; }
        if (opCode == 0x305) { opText = "SET WAIT FRAMES"; }
        if (opCode == 0x306) { opText = "WAIT"; }
        if (opCode == 0x307) { opText = "SET CONTROL LOCK"; }
        if (opCode == 0x30c) { opText = "ENTER VEHICLE"; }
        if (opCode == 0x318) { opText = "ENTER FIELD"; }
        if (opCode == 0x32b) { opText = "SET BATTLE LOCK"; }
        if (opCode == 0x330) { opText = "SET ACTIVE ENTITY"; }
        if (opCode == 0x333) { opText = "ROTATE ENTITY TO MODEL"; }
        if (opCode == 0x334) { opText = "WAIT FOR FUNCTION"; }

        if (opCode >= 0x204 && opCode < 0x300)
        {
            uint16_t funcID = opCode - 0x204;
            opText = "RUN FUNCTION " + std::to_string(funcID);
        }

        uintptr_t jumpAddress = ((startAddress + opCodeOffset) - WorldOffsets::JumpStart) / 2;

        if (opText == "")
        {
            LOG("0x%06x %04x Line %02d: Unknown. OpCode: %04x", startAddress + opCodeOffset, jumpAddress, line + 1, opCodes[i]);
        }
        else 
        {
            LOG("0x%06x %04x Line %02d: %s", startAddress + opCodeOffset, jumpAddress, line + 1, opText.c_str());
        }

        line++;
    }

    delete[] data;
}

void ScriptUtilities::findWorldScripts(GameManager* game, uintptr_t startAddress, uintptr_t endAddress)
{
    uintptr_t opCodeCount = endAddress - startAddress / 2;

    uint16_t* data = new uint16_t[opCodeCount];
    game->read(startAddress, opCodeCount * sizeof(uint16_t), (uint8_t*)(data));

    int prevFunctionEnd = 0;
    for (int i = 0; i < opCodeCount; ++i)
    {
        if (i < 3)
        {
            continue;
        }

        uint16_t opCode = data[i];
        uint16_t prev0 = data[i - 1];
        uint16_t prev1 = data[i - 2];
        uint16_t prev2 = data[i - 3];

        if (opCode == 0x318 && prev1 == 0x110)
        {
            uintptr_t address = startAddress + (prevFunctionEnd * 2);
            uintptr_t offset = address - WorldOffsets::ScriptStart;
            LOG("Found World Map Entrance: %d %d %04x", prev2, address, offset);
        }

        if (opCode == 0x203)
        {
            prevFunctionEnd = i + 1;
        }
    }
}