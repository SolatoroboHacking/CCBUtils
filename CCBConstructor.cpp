/*/////////////////////////////////////////////////////////////////

CCB Constructor

Copyright 2023 (C) - SolatoroboHacking

This program is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License as 
published by the Free Software Foundation, either version 3 of the 
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program. 
If not, see <https://www.gnu.org/licenses/>. 

*//////////////////////////////////////////////////////////////////

/*/////////////////////////////////////////////

The following defined constants and the LZ11Encode
and RLEEncode functions were originally written
in C by Github user PeterLemon. The original code
has been modified for use in this project. All
authoriship credit for these sections of code goes
to the original author, and this program is distributed
in compliance with the terms of the original GPLv3
license used by the original code. A link to the
appropriate repository can be found below. The
code's original copyright notices have also been
included below.

https://github.com/PeterLemon/Nintendo_DS_Compressors

*//////////////////////////////////////////////

/*----------------------------------------------------------------------------*/
/*--  lzx.c - LZ eXtended coding for Nintendo GBA/DS                        --*/
/*--  Copyright (C) 2011 CUE                                                --*/
/*--                                                                        --*/
/*--  This program is free software: you can redistribute it and/or modify  --*/
/*--  it under the terms of the GNU General Public License as published by  --*/
/*--  the Free Software Foundation, either version 3 of the License, or     --*/
/*--  (at your option) any later version.                                   --*/
/*--                                                                        --*/
/*--  This program is distributed in the hope that it will be useful,       --*/
/*--  but WITHOUT ANY WARRANTY; without even the implied warranty of        --*/
/*--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          --*/
/*--  GNU General Public License for more details.                          --*/
/*--                                                                        --*/
/*--  You should have received a copy of the GNU General Public License     --*/
/*--  along with this program. If not, see <http://www.gnu.org/licenses/>.  --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--  rle.c - RLE coding for Nintendo GBA/DS                                --*/
/*--  Copyright (C) 2011 CUE                                                --*/
/*--                                                                        --*/
/*--  This program is free software: you can redistribute it and/or modify  --*/
/*--  it under the terms of the GNU General Public License as published by  --*/
/*--  the Free Software Foundation, either version 3 of the License, or     --*/
/*--  (at your option) any later version.                                   --*/
/*--                                                                        --*/
/*--  This program is distributed in the hope that it will be useful,       --*/
/*--  but WITHOUT ANY WARRANTY; without even the implied warranty of        --*/
/*--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          --*/
/*--  GNU General Public License for more details.                          --*/
/*--                                                                        --*/
/*--  You should have received a copy of the GNU General Public License     --*/
/*--  along with this program. If not, see <http://www.gnu.org/licenses/>.  --*/
/*----------------------------------------------------------------------------*/

#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <array>

using namespace std;

class archiveFile {
	public:
		string name;
		uint32_t uncompressedSize;
		uint32_t compressedSize;
		unsigned char compressionType;
		unsigned char fileType;
		unsigned char* compressedData;
};

#define CMD_CODE_11   0x11       // LZX big endian magic number
#define CMD_CODE_40   0x40       // LZX low endian magic number
#define CMD_CODE_30   0x30       // RLE magic number

#define RLE_THRESHOLD 2          // max number of bytes to not encode
#define RLE_N         0x80       // max store, (RLE_LENGTH + 1)
#define RLE_F         0x82       // max coded, (RLE_LENGTH + RLE_THRESHOLD + 1)

#define RLE_MASK      0x80       // bits position:
                                 // ((((1 << RLE_CHECK) - 1) << (8 - RLE_CHECK)

#define LZX_SHIFT     1          // bits to shift
#define LZX_MASK      0x80       // first bit to check
                                 // ((((1 << LZX_SHIFT) - 1) << (8 - LZX_SHIFT)

#define LZX_VRAM      0x01       // VRAM file compatible (1)

#define LZX_THRESHOLD 2          // max number of bytes to not encode
#define LZX_N         0x1000     // max offset (1 << 12)
#define LZX_F         0x10       // max coded (1 << 4)
#define LZX_F1        0x110      // max coded ((1 << 4) + (1 << 8))
#define LZX_F2        0x10110    // max coded ((1 << 4) + (1 << 8) + (1 << 16))

unsigned char *LZ11Encode(unsigned char *raw_buffer, int raw_len, uint32_t *new_len, int cmd) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, *flg;
  unsigned int   pak_len, max, len, pos, len_best, pos_best;
  unsigned int   len_next, pos_next, len_post, pos_post;
  unsigned char  mask;

  int lzx_vram = LZX_VRAM;

#define SEARCH(l,p) {                                                          \
                    l = LZX_THRESHOLD - 1;                                     \
                                                                               \
                    max = raw - raw_buffer >= LZX_N ? LZX_N-1 : raw-raw_buffer;\
                    for (pos = lzx_vram + 1; pos <= max; pos++) {              \
                      for (len = 0; len < LZX_F2 - 1; len++) {                 \
                        if (raw + len == raw_end) break;                       \
                        if (*(raw + len) != *(raw + len - pos)) break;         \
                      }                                                        \
                                                                               \
                      if (len > l) {                                           \
                        p = pos;                                               \
                        if ((l = len) == LZX_F2 - 1) break;                    \
                      }                                                        \
                    }                                                          \
                    }

  pak_len = raw_len + ((raw_len + 7) / 8) + 3;
  pak_buffer = new unsigned char[pak_len];

  *(unsigned int *)pak_buffer = cmd | (raw_len << 8);

  pak = pak_buffer;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  mask = 0;

//------------------------------------------------------------------------------
//LZ11: - if x>1: xA BC <-------- copy ('x'   +  0x1) bytes from -('ABC'+1)
//      - if x=0: 0a bA BC <----- copy ('ab'  + 0x11) bytes from -('ABC'+1)
//      - if x=1: 1a bc dA BC <-- copy ('abcd'+0x111) bytes from -('ABC'+1)
//------------------------------------------------------------------------------
  if (cmd == CMD_CODE_11) {
    while (raw < raw_end) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      len_best = LZX_THRESHOLD;

      pos = raw - raw_buffer >= LZX_N ? LZX_N : raw - raw_buffer;
      for ( ; pos > lzx_vram; pos--) {
        for (len = 0; len < LZX_F2; len++) {
          if (raw + len == raw_end) break;
          if (*(raw + len) != *(raw + len - pos)) break;
        }

        if (len > len_best) {
          pos_best = pos;
          if ((len_best = len) == LZX_F2) break;
        }
      }

      if (len_best > LZX_THRESHOLD) {
        raw += len_best;
        *flg |= mask;
        if (len_best > LZX_F1) {
          len_best -= LZX_F1 + 1;
          *pak++ = 0x10 | (len_best >> 12);
          *pak++ = (len_best >> 4) & 0xFF;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        } else if (len_best > LZX_F) {
          len_best -= LZX_F + 1;
          *pak++ = len_best >> 4;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        } else{
          len_best--;
          *pak++ = ((len_best & 0xF) << 4) | ((pos_best - 1) >> 8);
          *pak++ = (pos_best - 1) & 0xFF;
        }
      } else {
        *pak++ = *raw++;
      }
    }
//------------------------------------------------------------------------------
//LZ40: - if x>1: Cx AB <-------- copy ('x'   +  0x0) bytes from -('ABC'+0)
//      - if x=0: C0 AB ab <----- copy ('ab'  + 0x10) bytes from -('ABC'+0)
//      - if x=1: C1 AB cd ab <-- copy ('abcd'+0x110) bytes from -('ABC'+0)
//------------------------------------------------------------------------------
  } else {
    while (raw < raw_end) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      SEARCH(len_best, pos_best);

      if (len_best >= LZX_THRESHOLD) {
        raw += len_best;
        SEARCH(len_next, pos_next);
        raw -= len_best - 1;
        SEARCH(len_post, pos_post);
        raw--;

        if (len_best + len_next <= 1 + len_post) len_best = 1;
      }

      if (len_best >= LZX_THRESHOLD) {
        raw += len_best;
        *flg = -(-*flg | mask);
        if (len_best > LZX_F1 - 1) {
          len_best -= LZX_F1;
          *pak++ = ((pos_best & 0xF) << 4) | 1;
          *pak++ = pos_best >> 4;
          *pak++ = len_best & 0xFF;
          *pak++ = len_best >> 8;
        } else if (len_best > LZX_F - 1) {
          len_best -= LZX_F;
          *pak++ = (pos_best & 0xF) << 4;
          *pak++ = pos_best >> 4;
          *pak++ = len_best;
        } else {
          *pak++ = ((pos_best & 0xF) << 4) | len_best;
          *pak++ = pos_best >> 4;
        }
      } else {
        *pak++ = *raw++;
      }
    }

    if (cmd == CMD_CODE_40) {
      if (!(mask >>= LZX_SHIFT)) {
        *(flg = pak++) = 0;
        mask = LZX_MASK;
      }

      *flg = -(-*flg | mask);
      *pak++ = 0;
      *pak++ = 0;
    }
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

unsigned char *RLEEncode(unsigned char *raw_buffer, int raw_len, uint32_t *new_len) {
  unsigned char *pak_buffer, *pak, *raw, *raw_end, store[RLE_N];
  unsigned int   pak_len, len, store_len, count;

  pak_len = 4 + raw_len + ((raw_len + RLE_N - 1) / RLE_N);
  pak_buffer = new unsigned char[pak_len];

  *(unsigned int *)pak_buffer = CMD_CODE_30 | (raw_len << 8);

  pak = pak_buffer;
  raw = raw_buffer;
  raw_end = raw_buffer + raw_len;

  store_len = 0;
  while (raw < raw_end) {
    for (len = 1; len < RLE_F; len++) {
      if (raw + len == raw_end) break;
      if (*(raw + len) != *raw) break;
    }

    if (len <= RLE_THRESHOLD) store[store_len++] = *raw++;

    if ((store_len == RLE_N) || (store_len && (len > RLE_THRESHOLD))) {
      *pak++ = store_len - 1;
      for (count = 0; count < store_len; count++) *pak++ = store[count];
      store_len = 0;
    }

    if (len > RLE_THRESHOLD) {
      *pak++ = RLE_MASK | (len - (RLE_THRESHOLD + 1));
      *pak++ = *raw;
      raw += len;
    }
  }
  if (store_len) {
    *pak++ = store_len - 1;
    for (count = 0; count < store_len; count++) *pak++ = store[count];
  }

  *new_len = pak - pak_buffer;

  return(pak_buffer);
}

// Main function
int main(int argc, char** argv) {
	// Check if input file has been provided
	if (argc < 2) {
		cout << "Error: Not enough arguments!" << endl;
		return 1;
		// Check if input file exists
	} else if (!filesystem::exists(strcat(argv[1], ".lst"))) {
		cout << "Error: input file does not exist!" << endl;
		return 1;
	} else {
		// Open the input file
		fstream lstFile;
		lstFile.open(argv[1],ios::in);
		string currentLine;
		// Get the number of files in the archive
		getline(lstFile, currentLine);
		int fileCount = stoi(currentLine);
		// Get the version number of the archive
		getline(lstFile, currentLine);
		int version = stoi(currentLine);
		archiveFile fileList[fileCount];

		// Print the name of the archive
		cout << endl << "Name of archive: " << (string(argv[1]).substr(0, string(argv[1]).find(".lst"))) << endl << endl;

		// For every file in the archive,
		for (int i = 0; i < fileCount; i++) {

			// Print file number
			cout << "File " << i + 1 << ":" << endl;

			// Read its name from the list, store it and print it
			getline(lstFile, currentLine);
			fileList[i].name = currentLine.substr(0, currentLine.find(" "));
			cout << "Name: " << fileList[i].name << endl;

			// Erase the name from the current line
			currentLine.erase(0, currentLine.find(" ") + 1);

			// Read and store the file type value. The function of this value is currently not fully understood,
			// so it is not printed. It is stored because it is needed to create a valid header.
			fileList[i].fileType = ((unsigned char)stoi(currentLine.substr(0, currentLine.find(" ")))) & 0xFF;

			// Erase the file type value from the current line
			currentLine.erase(0, currentLine.find(" ") + 1);

			// Read, store and print the compression type value
			fileList[i].compressionType = ((unsigned char)stoi(currentLine.substr(0, currentLine.find(" ")))) & 0xFF;
			cout << "Compression Type: 0x" << setw(2) << setfill('0') << hex << (fileList[i].compressionType & 0xFF);
			if (fileList[i].compressionType == 0x11 || fileList[i].compressionType == 0x40) {
				// Compression type 0x11 and 0x40 are different versions of LZ11 compression
				cout << " (LZ11 Compression)";
			} else if (fileList[i].compressionType == 0x30) {
				// Compression type 0x30 is RLE/RLUncomp compression
				cout << " (RLUncomp Compression)";
			}
			cout << endl;

			// Erase the compression type value from the current line
			currentLine.erase(0, currentLine.find(" ") + 1);

			// Read, store and print the uncompressed file size
			// The compressed size cannot be printed because it is not known until the file has been compressed
			fileList[i].uncompressedSize = (uint32_t)stoul(currentLine);
			cout << "Uncompressed Size: " << dec << fileList[i].uncompressedSize << " bytes" << endl << endl;
		}

		// Open a new CCB file for writing
		FILE* ccbFile = fopen((string(argv[1]).substr(0, string(argv[1]).find(".lst"))).c_str(), "wb");

		// Constructing CCB header byte by byte

		// The first four bytes are an identifier of 'CCB '
		fputc('C', ccbFile);
		fputc('C', ccbFile);
		fputc('B', ccbFile);
		fputc(' ', ccbFile);

		// The next two bytes are the version number
		fputc((unsigned char)version, ccbFile);
		fputc((unsigned char)(version >> 8), ccbFile);

		// The following two bytes are the number of files in the archive
		fputc((unsigned char)fileCount, ccbFile);
		fputc((unsigned char)(fileCount >> 8), ccbFile);

		// The next part of the header is the file table.
		// The file table describes the attributes of each file
		for (int i = 0; i < fileCount; i++) {
			// First comes the name of the file
			fwrite(fileList[i].name.c_str(), 1, fileList[i].name.length(), ccbFile);

			// The file name may only be up to 24-bytes in length.
			// If the name is shorter, it will be padded with null bytes.
			for (int j = 0; j < 24-fileList[i].name.length(); j++) {
				fputc('\0', ccbFile);
			}

			// The next byte is the file type value.
			// It is not fully understood, but it seems as though
			// most files of certain types have the same value
			fputc(fileList[i].fileType, ccbFile);

			// Open the current file for reading
			FILE* currentFile = fopen(fileList[i].name.c_str(), "rb");

			// Create a new buffer for its uncompressed data according to the size found
			// in the file list.
			unsigned char* uncompressedData = new unsigned char[fileList[i].uncompressedSize];

			// Read in the entire file
			fread(uncompressedData, 1, fileList[i].uncompressedSize, currentFile);

			// Determine the compression type and pass it to its appropriate compression function.
			// The files are read and compressed at this point because the next required value in the
			// file table is the file's compressed size, and that cannot be known until the file
			// has been compressed. The compressed data is not written at this point, just stored.
			if (fileList[i].compressionType == 0x11 || fileList[i].compressionType == 0x40) {
				// LZ11-compressed files are passed to the LZ11Encode function
				fileList[i].compressedData = LZ11Encode(uncompressedData, fileList[i].uncompressedSize, &fileList[i].compressedSize, fileList[i].compressionType);
			} else if (fileList[i].compressionType == 0x30) {
				// RLE-compressed files are passed to the RLEEncode function
				fileList[i].compressedData = RLEEncode(uncompressedData, fileList[i].uncompressedSize, &fileList[i].compressedSize);
			} else if (fileList[i].compressionType == 0x00) {
				// Some files within CCB archives are completely uncompressed.
				// These files are written with no modification.
				fileList[i].compressedData = uncompressedData;
				fileList[i].compressedSize = fileList[i].uncompressedSize;
			}

			// The next three bytes of the file table are the compressed size
			// of the current file.
			fputc((unsigned char)fileList[i].compressedSize, ccbFile);
			fputc((unsigned char)(fileList[i].compressedSize >> 8), ccbFile);
			fputc((unsigned char)(fileList[i].compressedSize >> 16), ccbFile);

			// The next byte is the compression type of the file.
			fputc(fileList[i].compressionType, ccbFile);

			// The final three bytes are the uncompressed size of the file
			fputc((unsigned char)fileList[i].uncompressedSize, ccbFile);
			fputc((unsigned char)(fileList[i].uncompressedSize >> 8), ccbFile);
			fputc((unsigned char)(fileList[i].uncompressedSize >> 16), ccbFile);

			// Each file has its own section in the file table, always in 
			// the above-referenced order.

			// Deleting uncompressed data, as it is no longer necessary
			delete uncompressedData;
		}

		// After the file table is the raw compressed file.
		// Normally, files compressed with LZ11 or RLE have a four-byte header
		// containing the uncompressed size of the file. Files contained in a
		// CCB archive do not have this four-byte header because that information
		// is found within the file table of the CCB archive.

		for (int i = 0; i < fileCount; i++) {
			fwrite(fileList[i].compressedData, 1, fileList[i].compressedSize, ccbFile);
			delete fileList[i].compressedData;
		}
	}
}
