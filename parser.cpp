#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <set>
#include <sstream>
#include <cstring>
#include <stdint.h>

using namespace std;

typedef  vector<char> packetv_t;

class Packet
{
    public:
        Packet():_packetLen(0), _packetType(0), _msgId(0){};

        void push_back(char iVal){ _packv.push_back(iVal); };
        void clear(){ _packv.clear(); };
        string toString() const;

        void parseHex();

        const uint32_t getPacketType() const { return _packetType; };


    private:
        packetv_t _packv;      // raw packed data stored in vector of chars
        uint8_t  _packetLen;   // 1 byte - packet length
        uint16_t _packetType;  // 3,4 bytes - type of packet (6C5C for message beginning)
        uint32_t _msgId;       // 6-9 bytes - message ID 
};


string Packet::toString() const
{
    stringstream ss;
    ss << "PacketID: " << dec << _msgId;
    ss << ", len: " << (int)_packetLen;
    ss << ", type: " << noshowbase << hex << setw(4) << setfill ('0') << _packetType << ": ";
    for ( packetv_t::const_iterator it = _packv.begin(); it != _packv.end(); ++it)
    {
        ss << noshowbase << hex << setw(2) << setfill('0') << (int)(unsigned char)(*it) << " ";
    }
    ss << dec << endl;
    return ss.str();
}


void Packet::parseHex()
{
// Examples of hex packet: 
//    55 1c 00 7a 14 00 6d f7 01 00 dd 63 dd 63 dd 63 dd 63 61 0d 6c 4d 65 27 6f 4d fe b9 
//    55 28 00 6c 5c 00 6e f7 01 00 92 91 70 6e 91 91 0c 6e 69 6e 61 96 6e 6e 6e 6e 6e 6e 6e 6e 6e 6e 6e 6e 52 83 6f 6e 40 df 
//    55 28 00 6c 5c 00 6f f7 01 00 69 6f 72 6f 92 90 0e 6f 67 6f 7e 97 6f 6f 6f 6f 6f 6f 6f 6f 6f 6f 6f 6f 52 82 6e 6f e6 9a 
//    55 28 00 6c 5c 00 70 f7 01 00 70 70 6e 70 71 70 10 70 78 70 62 88 70 70 70 70 70 70 70 70 70 70 70 70 4e 9d 71 70 60 a7 
//     |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
//     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15     -- byte number
//
//  0   - packet beginning
//  1   - packet length (including 0 byte = 0x55)
//  2   - unknown, always 0 (maybe part of packet id or reserved for future use)
//  3,4 - packet type. Many packets of different type  can be related to one message (see bytes 6-9)
//        packet of one type has olways same size
//        apparently bytes 3 and 4 have different meanings but they are related to each other
//       *untill we understand their meaning we consider them as one 2bytes int
//          - 6c5c - always first packet in the message
//          - other values - meaning unknown yet
//  5   - unknown, almost always 0, can be 0xFF when byte 4 is 0xFF (probably means error)
//  6-9 - seems to be an ID of a message (group of packets)
//  
//  other byts are unknown and we consider them as DATA (or payload)
//

    if (_packv.size() < 10)
    {
        cout << "Error while parsing message: message to short, size: " << dec <<_packv.size() << endl;
        return;
    }
    // byte 1 - packet len
    _packetLen = _packv[1];
    
    // bytes 3 and 4 - packet type (2 bytes)
    char aType[2];
    for (int i = 0; i<2; ++i) aType[i] = _packv[i+3];
    memcpy(&_packetType, aType, 2);

    // bytes 6-9 - message ID (4 bytes)
    char aMsgId[4];
    for (int i = 0; i<4; ++i)
    {
        aMsgId[i] = _packv[i+6];
    }
    memcpy(&_msgId, aMsgId, 4);
}


//-----------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    if (argc < 2 )
    {
        cout << "You should provide filename as an argument" << endl;
        return 1;
    }
    int num = 1;
    if(*(char *)&num == 1)
    {
        cout << "Little-Endian"<<endl;
    }
    else
    {
        cout << "Big-Endian"<<endl;
        return 1; // exit because initial files are little endian and we don't support endianness changing
    }
    string filename(argv[1]);
    cout << "Reading file " << filename << endl;

    ifstream file(filename.c_str(), ios::binary);
    if (file.fail() || !file.is_open())
    {
        cout << "Cannot open file " << filename << endl;
        return 1;
    }

    char aBuf;
    Packet packet;
    vector<Packet> packets;
    while (!file.eof() && !file.fail()) 
    {
        file.read(&aBuf, 1);
        if (aBuf == 0x55)
        {
            // 0x55 is packet beginning only if there is a mask like 55XX00
            // check that 55XX00 mask exist for current 0x55 byte
            streampos aPos = file.tellg();
            // read 2 more bytes
            char aTmpBuf;
            file.read(&aTmpBuf, 1);
            file.read(&aTmpBuf, 1);
            if (aTmpBuf == 0x00)
            {
                packet.parseHex();
                cout << packet.toString();
                packets.push_back(packet);
                packet.clear();
            }
            // return back the position
            file.seekg(aPos);
        }
        packet.push_back(aBuf);
    }
    cout << "Number of packets found: " << dec <<  packets.size() << endl;
    set<uint32_t> idset;
    for (vector<Packet>::const_iterator it = packets.begin(); it != packets.end(); ++it)
    {
        idset.insert(it->getPacketType());
    }

    cout << "Number of different packet IDs found: " << dec << idset.size() << endl;
    for (set<uint32_t>::iterator it = idset.begin(); it != idset.end(); ++it)
    {
        cout << noshowbase << hex << setw(4) << setfill ('0') << *it << " ";
    } 
    cout << endl;

return 0;
} 
