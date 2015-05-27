#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <cstring>
#include <math.h>

#include "packet.h"

using namespace std;


//====== Class PACKET ======

//static methods
bool Packet::isDoubleGpsLike(const double& iDouble)
{
    // check square where gps coordinates should point from our DAT files.
//    return (iDouble > 13.0 && iDouble < 102.0 && fabs(iDouble)>0.01);
return (iDouble > -1000 && iDouble < 3000 && fabs(iDouble)>0.01);
}

double Packet::convertDoubleToGps(const double& iDouble)
{
    return iDouble * 180.0 / 3.141592653589793; // 0x32 at github.com/noahwilliamsson/dji-phantom-vision/blob/master/dji-phantom.c
}

double Packet::doubleFromBytes(const uint8_t* iBytes)
{
    double res;
    memcpy(&res, iBytes, sizeof(double));
    return res;
}

double Packet::floatFromBytes(const uint8_t* iBytes)
{
    double res;
    memcpy(&res, iBytes, sizeof(float));
    return res;
}

const double Packet::doubleFromBytePos(int iPosition) const
{
    uint8_t aBytes[8]; // 8 - sizeof(double)

    // copy 8 bytes to aBytes
    for (int j = 0; j<sizeof(double); ++j)
    {
        aBytes[j] = _packv[j];
    }
    // convert aBytes to double
   return  Packet::doubleFromBytes(aBytes);
}

const double Packet::floatFromBytePos(int iPosition) const
{
    uint8_t aBytes[4]; // 4 - sizeof(float)

    // copy 4 bytes to aBytes
    for (int j = 0; j<sizeof(float); ++j)
    {
        aBytes[j] = _packv[j];
    }
    // convert aBytes to double
   return  Packet::floatFromBytes(aBytes);
}



//member methods
void Packet::dataToDoubles()
{
    uint8_t aBytes[8]; // 8 - sizeof(double)

    for (int i = PACKET_HEADER_LEN; i<_packv.size()-7; ++i)
    {
        // copy 8 bytes to aBytes
        for (int j = 0; j<8; ++j)
        {
            aBytes[j] = _packv[i+j];
        }
        // convert aBytes to double
        _dataDoubles.push_back(doubleFromBytes(aBytes));
    }
}


string Packet::toString() const
{
    stringstream ss;
    ss << "MsgID: " << dec << _msgId;
    ss << ", len: " << (int)_packetLen;
    ss << ", type: " << noshowbase << hex << setw(4) << setfill ('0') << _packetType << endl;
    for ( packetv_t::const_iterator it = _packv.begin(); it != _packv.end(); ++it)
    {
        ss << noshowbase << hex << setw(2) << setfill('0') << (int)(unsigned char)(*it) << " ";
    }
    ss << dec << endl;
    return ss.str();
}


void Packet::parseHex()
{
    if (_packv.size() < PACKET_HEADER_LEN)
    {
        cerr << "Error while parsing message: message to short, size: " << dec <<_packv.size() << endl;
        _valid = false;
        return;
    }

    // byte 1 - packet len
    _packetLen = _packv[1];
    // packet len in byte1 must be the same as _packv vector length
    // otherwise consider that message is corrupted
    if (_packetLen != _packv.size() )
    {
        cerr << "Error while parsing message: message length in packet ("<< dec << (int)_packetLen
             << ") is not equal to number of bytes read from file("<< dec << _packv.size() << ")" << endl;
        _valid = false;
        return;
    }

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

    _valid = true;

    // try to convert all data bytes into doubles
    dataToDoubles();
}



//====== class GPSPacket ======
void GPSPacket::parseHex()
{
    Packet::parseHex();
    
    // first we should check packet type (must be 0x01CF)
    if (this->getPacketType() != 0x01CF)
    {
        cout << "GPSPacket: Wrong packet type: " << hex << this->getPacketType();
        _valid = false;
    }

    // then check byte#6 (must be 0x00 for pgs packet)
    if (this->getPacketRaw()[6] != 0x00)
    {
        cout << "GPSPacket: Wrong byte#6 (muxt be 0x00) " << hex << this->getPacketRaw()[6] << endl;
        _valid = false;
    }

    // now we can get lat/long/alt values
    // TODO

}



