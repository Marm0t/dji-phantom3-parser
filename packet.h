#ifndef PACKET_H_
#define PACKET_H_

#include <vector>
#include <stdint.h>

#define PACKET_START      0x55
#define PACKET_HEADER_LEN 10


typedef  std::vector<uint8_t> packetv_t;

// Examples of hex packet: 
//    55 28 00 6c 5c 00 70 f7 01 00 70 70 6e 70 71 70 10 70 78 70 62 88 70 70 70 70 70 70 70 70 70 70 70 70 4e 9d 71 70 60 a7 
//     |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
//     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15     -- byte number
//
//  0   - packet beginning
//  1   - packet length (including 0 byte = 0x55)
//  2   - unknown, always 0 (maybe part of packet id or reserved for future use)
//  3,4 - packet type. Many packets of different type  can be related to one message (see bytes 6-9)
//        packet of one type has always same size
//        apparently bytes 3 and 4 have different meanings but they are related to each other
//       *untill we understand their meaning we consider them as one 2bytes int
//          - 6c5c - always first packet in the message
//          - other values - meaning unknown yet
//  5   - unknown, almost always 0, can be 0xFF when byte 4 is 0xFF (probably means error)
//  6-9 - seems to be an ID of a message (group of packets)
//  
//  other byts are unknown and we consider them as DATA (or payload)
class Packet
{

    public:
        Packet():_packetLen(0), _packetType(0), _msgId(0){};
        Packet(packetv_t iByteVector):_packv(iByteVector){};

        void push_back(char iVal){ _packv.push_back(iVal); };
        void clear(){ _packv.clear(); _dataDoubles.clear(); };
        std::string toString() const;

        void parseHex();

        const uint32_t getPacketType() const { return _packetType; };
        const std::vector<double>& getDataDoubles() {return _dataDoubles; } ;

        static double doubleFromBytes(const uint8_t* iBytes);
        static bool   isDoubleGpsLike(const double& iDouble); // checks if double value can be interpreted as GPS coordinates
        static double convertDoubleToGps(const double& iDOuble); // applies rule from dji phantom2 protocole for gps coordinates value

    protected:
        packetv_t _packv;      // raw packed data stored in vector of chars
        uint8_t  _packetLen;   // 1 byte - packet length
        uint16_t _packetType;  // 3,4 bytes - type of packet (6C5C for message beginning)
        uint32_t _msgId;       // 6-9 bytes - message ID 
        
        // used before to decode packet format
         std::vector<double> _dataDoubles; // vector for data parsed to doubles 
         void dataToDoubles();
};

#endif // #ifndef PACKET_H_
