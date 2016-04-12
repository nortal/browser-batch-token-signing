/*
 * Chrome Token Signing Native Host
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <string>
#include <vector>

#include <winscard.h>


#define MAX_ATR_SIZE 33	/**< Maximum ATR size */


class CardReader {
public:
    CardReader(const std::wstring &_name, SCARDCONTEXT _context) : name(_name), context(_context){}
    void connect();
    void populateAtr();
    void release();
    std::wstring getName();
    std::string getAtr();
private:
    std::wstring name;
    std::string atr;
    SCARDHANDLE cardHandle;
    SCARDCONTEXT context;
    DWORD activeProtocol;
};

class AtrFetcher {
public:
    std::vector<std::string> fetchAtr();
private:
    void populate();
    void establishContext();
    void listReaders();
    void populateAtrs();
    void release();
    
    std::vector<std::string> atrs;
    LONG error;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    std::vector<CardReader*> readerList;
};

