#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdint.h>

#include "packet.h"


using namespace std;


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
        cout << "Big-Endian: exit because initial files are little endian and we're too lazy to do it by ourselves"<<endl;
        return 1;
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
    int packetsNumber = 0;
    vector<Packet> packets01cf;
    streampos aPos;
    //ignore everything till beginning of first packet (0x55 marker)
    while(!file.eof() && !file.fail() && aBuf != 0x55)
    {
        aPos = file.tellg();
        file.read(&aBuf, 1); // read one byte
    cout << "[DBG]: ABUF=0x"<<hex << (unsigned int)aBuf << endl;
        
    }
    if (file.eof() || file.fail())
    {
        cout << "No dji-phantom-3 packets found in your file!" << endl;
        return 1;
    }
    // go back one byte in the file so we start reading packet from the beginning
    file.seekg(aPos);

    // now lets read all packets one by one
    while (!file.eof() && !file.fail()) 
    {
        // packet example:
        //# 0  1  2  3  4  5  6  7 ...
        // 55 28 00 6c 5c 00 70 f7 01 00 70 70 6e 70 71 70 10 70 78 70 62 88 70 70 70 70 70 70 70 70 70 70 70 70 4e 9d 71 70 60 a7
        //  ^ - we start from here and should read LEN bytes
        // LEN = byte#1 (in example above it is 0x28 (hex) = 40 (dec)
        Packet packet;
        file.read(&aBuf, 1);
        if (aBuf != 0x55)
        {
            // this happens very often especially at the beginning of file
            //cout << "Wrong packet start marker: " << hex << (int)aBuf << endl;
            continue;
        }
        packet.push_back(aBuf);

        // reading byte#1 (packet length)
        file.read(&aBuf, 1);
        uint8_t packetLen = aBuf;
        if (packetLen < PACKET_HEADER_LEN)
        {
            cout << "Packet length is too small (" << dec << packetLen << "), packet corrupted" << endl;
            continue;
        }
        packet.push_back(aBuf);
        
        // reading byte#2: must be always 0x00!
        file.read(&aBuf, 1);
        if (aBuf != 0x00)
        {
            cout << "Corrupted packet header: byte#2 is not 0x00: " << hex << (int)aBuf << endl;
            continue;
        }
        packet.push_back(aBuf);

        // now lets read the rest of the packet accordint to packet len from byte#1
        // note thaat we have already read 3 first bytes (#0, #1 and #3)
        for (uint8_t i=3; i<packetLen; ++i)
        {
            file.read(&aBuf, 1);
            packet.push_back(aBuf);
        }

        // Packet is ready for parsing!
        packet.parseHex();

        // check the theory that packet of type 0x01cf is dedicated
        // for gps coordinates
        if (packet.getPacketType() == 0x01cf)
        {
            packets01cf.push_back(packet);
            
            // lets read all doubles and select only those packets
            // that have more than 1
            Packet& tp = packets01cf.back();
            int found = 0;
            stringstream ss;
            for (vector<double>::const_iterator it = tp.getDataDoubles().begin(); 
                    it != tp.getDataDoubles().end(); ++it)
            {
                if (Packet::isDoubleGpsLike(Packet::convertDoubleToGps(*it)))
                {
                    ss << "Converted double found at position [" << dec << it-tp.getDataDoubles().begin()
                         << "] " << dec << fixed << Packet::convertDoubleToGps(*it) << endl;
                    found++;
                }
            }
            if (found>1 /*&& fabs(Packet::convertDoubleToGps(tp.getDataDoubles()[0])-99) < 2*/   ) // TODO: this if is just for analysis, like grep 
            {
                cout << ss.str();
                cout << tp.toString();
                // NOTE: not all of these packets pointing to the right place! 
                // structure of this packet still needs to be decoded
                // what we know now is that coordinates are stored in packets of type 0x01cf, 
                // gps pars starts from 0 byte of data (longitude) then 8th byte of data (lattitude)
                // the rest is to be decoded
                // also needs to be decoded why other packets of the same type & structure are
                // pointing to another places
            }
        }
        packetsNumber++;
    }

    cout << "Number of packets found: " << dec << packetsNumber << endl;
    cout << "Number of packets in packets01cf: " << dec << packets01cf.size() << endl;
    return 0;
} 
