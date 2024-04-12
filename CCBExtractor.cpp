/*/////////////////////////////////////////////////////////////////

CCB Extractor

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
#include <unistd.h>

/*/////////////////////////////////////////////

The following defined constants and the LZ11Decode
and RLEDecode functions were originally written
in C by Github user PeterLemon. The original code
has been modified for use in this project. All
authoriship credit for these sections of code goes
to the original author, and this program is distributed
in compliance with the terms of the original GPLv3
license used by the original code. A link to the
appropriate repository can be found below.

https://github.com/PeterLemon/Nintendo_DS_Compressors

*//////////////////////////////////////////////


#define CMD_CODE_10   0x10       // LZSS magic number
#define CMD_CODE_11   0x11       // LZX big endian magic number
#define CMD_CODE_40   0x40       // LZX low endian magic number

#define CMD_CODE_30   0x30       // RLE magic number

#define RLE_MASK      0x80       // bits position:
                                 // ((((1 << RLE_CHECK) - 1) << (8 - RLE_CHECK)

#define RLE_LENGTH    0x7F       // length, (0xFF & ~RLE_MASK)

#define LZX_SHIFT     1          // bits to shift
#define LZX_MASK      0x80       // first bit to check
                                 // ((((1 << LZX_SHIFT) - 1) << (8 - LZX_SHIFT)

#define LZS_SHIFT     1          // bits to shift
#define LZS_MASK      0x80       // bits to check:
                                 // ((((1 << LZS_SHIFT) - 1) << (8 - LZS_SHIFT)

#define LZS_THRESHOLD 2          // max number of bytes to not encode

#define RLE_THRESHOLD 2          // max number of bytes to not encode

#define LZX_THRESHOLD 2          // max number of bytes to not encode
#define LZX_F         0x10       // max coded (1 << 4)
#define LZX_F1        0x110      // max coded ((1 << 4) + (1 << 8))

using namespace std;

void LZ10Decode(string filename, unsigned char* compressedData, unsigned char* uncompressedData, unsigned int compressedSize, unsigned int uncompressedSize, unsigned char compressionType) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, header, len, pos;
  unsigned char  flags, mask;

  cout << "Decompressing " << filename << endl;

  pak_buffer = compressedData;

  header = compressionType;
  if (header != CMD_CODE_10) {
    cout << "WARNING: file is not LZSS encoded!" << endl;
    return;
  }

  pak_len = compressedSize;

  raw_len = uncompressedSize;
  raw_buffer = uncompressedData;

  pak = pak_buffer;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZS_SHIFT)) {
      if (pak == pak_end) break;
      flags = *pak++;
      mask = LZS_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) break;
      *raw++ = *pak++;
    } else {
      if (pak + 1 >= pak_end) break;
      pos = *pak++;
      pos = (pos << 8) | *pak++;
      len = (pos >> 12) + LZS_THRESHOLD + 1;
      if (raw + len > raw_end) {
        cout << "WARNING: wrong decoded length!" << endl;
        len = raw_end - raw;
      }
      pos = (pos & 0xFFF) + 1;
      while (len--) *raw++ = *(raw - pos);
    }
  }

  raw_len = raw - raw_buffer;

  if (raw != raw_end) printf(", WARNING: unexpected end of encoded file!");

  FILE* outputFile = fopen(filename.c_str(), "wb");
  fwrite(raw_buffer, sizeof(char), raw_len, outputFile);
}

void LZ11Decode(string filename, unsigned char* compressedData, unsigned char* uncompressedData, unsigned int compressedSize, unsigned int uncompressedSize, unsigned char compressionType) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   header, len, pos, threshold, tmp;
  unsigned char  flags, mask;
  int pak_len, raw_len;

  cout << "Decompressing " << filename << endl;

  pak_buffer = compressedData;

  header = compressionType;
  if ((header != CMD_CODE_11) && ((header != CMD_CODE_40))){
    cout << "WARNING: File is not LZX Encoded!" << endl;
    return;
  }

  pak_len = compressedSize;

  raw_len = uncompressedSize;
  raw_buffer = uncompressedData;

  pak = pak_buffer;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  mask = 0;

  while (raw < raw_end) {
    if (!(mask >>= LZX_SHIFT)) {
      if (pak == pak_end) {
      	break;
      }
      flags = *pak++;
      if (header == CMD_CODE_40) flags = -flags;
      mask = LZX_MASK;
    }

    if (!(flags & mask)) {
      if (pak == pak_end) {
      	break;
      }
      *raw++ = *pak++;
    } else {
      if (header == CMD_CODE_11) {
        if (pak + 1 >= pak_end) {
        	break;
        }
        pos = *pak++;
        pos = (pos << 8) | *pak++;

        tmp = pos >> 12;
        if (tmp < LZX_THRESHOLD) {
          pos &= 0xFFF;
          if (pak == pak_end) {
          	break;
          }
          pos = (pos << 8) | *pak++;
          threshold = LZX_F;
          if (tmp) {
            if (pak == pak_end) {
            	break;
            }
            pos = (pos << 8) | *pak++;
            threshold = LZX_F1;
          }
        } else {
          threshold = 0;
        }

        len = (pos >> 12) + threshold + 1;
        pos = (pos & 0xFFF) + 1;
      } else {
        if (pak + 1 == pak_end) {
        	break;
        }
        pos = *pak++;
        pos |= *pak++ << 8;

        tmp = pos & 0xF;
        if (tmp < LZX_THRESHOLD) {
          len = *pak++;
          threshold = LZX_F;
          if (tmp) {
            if (pak == pak_end) {
            	break;
            }
            len = (*pak++ << 8) | len;
            threshold = LZX_F1;
          }
        } else {
          len = tmp;
          threshold = 0;
        }

        len += threshold;
        pos >>= 4;
      }

      if (raw + len > raw_end) {
        cout << "WARNING: wrong decoded length!" << endl;
        len = raw_end - raw;
      }

      while (len--) *raw++ = *(raw - pos);
    }
  }

  if (header == CMD_CODE_40) pak += *pak == 0x80 ? 3 : 2;

  raw_len = raw - raw_buffer;

  if (raw != raw_end) {
  	cout << "WARNING: Unexpected end of encoded file" << endl;
  }
  FILE* outputFile = fopen(filename.c_str(), "wb");
  fwrite(raw_buffer, sizeof(char), raw_len, outputFile);
}

void RLEDecode(string filename, unsigned char* compressedData, unsigned char* uncompressedData, unsigned int compressedSize, unsigned int uncompressedSize, unsigned char compressionType) {
  unsigned char *pak_buffer, *raw_buffer, *pak, *raw, *pak_end, *raw_end;
  unsigned int   pak_len, raw_len, header, len;

  cout << "Decompressing " << filename << endl;

  pak_buffer = compressedData;
  pak_len = compressedSize;

  header = compressionType;
  if (header != CMD_CODE_30) {
    cout << "WARNING: file is not RLE encoded!" << endl;
  }

  raw_len = uncompressedSize;
  raw_buffer = uncompressedData;

  pak = pak_buffer;
  raw = raw_buffer;
  pak_end = pak_buffer + pak_len;
  raw_end = raw_buffer + raw_len;

  while (raw < raw_end) {
    len = *pak++;
    if (pak == pak_end) break;
    if (!(len & RLE_MASK)) {
      len = (len & RLE_LENGTH) + 1;
      if (raw + len > raw_end) {
        cout << "WARNING: wrong decoded length!" << endl;
        len = raw_end - raw;
      }
      if (pak + len > pak_end) {
        len = pak_end - pak;
      }
      while (len--) *raw++ = *pak++;
    } else {
      len = (len & RLE_LENGTH) + RLE_THRESHOLD + 1;
      if (raw + len > raw_end) {
        cout << "WARNING: wrong decoded length!" << endl;
        len = raw_end - raw;
      }
      while (len--) *raw++ = *pak;
      pak++;
    }
    if (pak == pak_end) break;
  }

  raw_len = raw - raw_buffer;

  if (raw != raw_end) {
  	cout << "WARNING: unexpected end of encoded file!" << endl;
  }

  FILE* outputFile = fopen(filename.c_str(), "wb");
  fwrite(raw_buffer, sizeof(char), raw_len, outputFile);
}


class archiveFile {
	public:
		string name;
		unsigned int compressedSize;
		unsigned int uncompressedSize;
		unsigned char compressionType;
		unsigned char fileType;
};

bool INFO_ONLY;
bool DISPLAY_HELP;
bool GENERATE_LST;
bool NEW_DIRECTORY;

void checkParams(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			DISPLAY_HELP = true;
		} else if (strcmp(argv[i], "-i") == 0) {
			INFO_ONLY = true;
		} else if (strcmp(argv[i], "-l") == 0) {
			GENERATE_LST = true;
		} else if (strcmp(argv[i], "-d") == 0) {
			NEW_DIRECTORY = true;
		}
	}
}

// Main function
int main(int argc, char** argv) {
	// Check command-line options
	checkParams(argc, argv);
	// If '-h' was passed at command line, display help
	if (DISPLAY_HELP) {
		cout << "CCB Extractor v1.0 - A utility to extract the proprietary CyberConnect2 Batch (CCB) files found within Solatorobo: Red the Hunter" << endl;
		cout << endl << "Usage: CCBExtractor [input_file] ([arg])" << endl;
		cout << endl << "Command-line arguments:" << endl;
		cout << endl << "-d      Extract files to new directory" << endl;
		cout << "-h      This help" << endl;
		cout << "-i      Display archive information without extracting files" << endl;
		cout << "-l      Generate .lst file for reconstructing CCB file" << endl;
		return 0;
	}
	// If no arguments were provided, give an error
	if (argc < 2) {
		cout << "Error: Not enough arguments" << endl;
		return 1;
		// If the input file does not exist, give an error
	} else if (!filesystem::exists(argv[1])) {
		cout << "Error: Input file does not exist" << endl;
		return 1;
	} else {
		// Create pointer for buffer read from file
		unsigned char* readBytes;
		// Open the input file for reading
		FILE* ccbFile = fopen(argv[1], "rb");
		// Create a 5-byte buffer to store identifier
		readBytes = new unsigned char[5];
		// Read the first 4 bytes of the file
		fread(readBytes, 1, 4, ccbFile); 
		// Terminate with a null byte to make a valid string
		readBytes[4] = '\0';
		// If the input file does not begin with 'CCB ', the file is not a valid CCB file
		if (strcmp(reinterpret_cast<const char*>(readBytes), "CCB ") != 0) {
			cout << "Error: Input file is not a valid CCB file" << endl;
			return 1;
		} else {
			// Print the name of the CCB archive
			cout << "Name of archive: " << argv[1] << endl;
			// Delete the old array
			delete readBytes;
			// Read the next two bytes
			readBytes = new unsigned char[2];
			fread(readBytes, 1, 2, ccbFile);
			// Store these bytes as the CCB version number
			int version = (int)(((unsigned char)readBytes[1] << 8) + (unsigned char)readBytes[0]);
			// Delete old array and read next 2 bytes
			delete readBytes;
			readBytes = new unsigned char[2];
			fread(readBytes, 1, 2, ccbFile);
			// Use these bytes as the number of files in the archive
			archiveFile fileList[(int)(((unsigned char)readBytes[1] << 8) + (unsigned char)readBytes[0])];
			// Print number of files in archive
			cout << endl << "Number of files in archive: " << sizeof(fileList)/sizeof(fileList[0]) << endl << endl;
			// Ensure that the file pointer is set correctly: purely precautionary
			fseek(ccbFile, 8, SEEK_SET);
			// For every file in the archive,
			for (int i = 0; i < sizeof(fileList)/sizeof(fileList[0]); i++) {
				// Delete the old array
				delete readBytes;
				// Read 24-byte filename
				readBytes = new unsigned char[24];
				fread(readBytes, 1, 24, ccbFile);
				// Store it in the file list as a string
				fileList[i].name = reinterpret_cast<char*>(readBytes);
				// Print the name and number of the file
				cout << "File " << (i+1) << ":" << endl;
				cout << endl << "Name: " << fileList[i].name << endl;
				// Store the next byte as the file type value
				// This value is not currently fully understood, so it is not printed
				delete readBytes;
				readBytes = new unsigned char[1];
				fread(readBytes, 1, 1, ccbFile);
				fileList[i].fileType = readBytes[0];
				delete readBytes;
				// Read the next three bytes
				readBytes = new unsigned char[3];
				fread(readBytes, 1, 3, ccbFile);
				// Convert, print and store them as the size of the compressed data
				fileList[i].compressedSize = (uint32_t)(((unsigned char)readBytes[2] << 16) | ((unsigned char)readBytes[1] << 8) | (unsigned char)readBytes[0]);
				cout << "Compressed File Size: " << fileList[i].compressedSize << " bytes" << endl;
				// Read the next byte
				delete readBytes;
				readBytes = new unsigned char[1];
				fread(readBytes, 1, 1, ccbFile);
				// Convert, print and store it as the compression type the file uses
				fileList[i].compressionType = readBytes[0];
				cout << "Compression type: 0x" << setw(2) << setfill('0') << hex << (fileList[i].compressionType & 0xff);
				if (fileList[i].compressionType == 0x11) {
					cout << " (LZ11 Compression)" << endl;
				} else if (fileList[i].compressionType == 0x10) {
					cout 	<< " (LZ10 Compression)" << endl;
				} else if (fileList[i].compressionType == 0x30) {
					cout << " (RLUncomp Compression)" << endl;
				} else {
					cout << endl;
				}
				// Read the next three bytes
				delete readBytes;
				readBytes = new unsigned char[3];
				fread(readBytes, 1, 3, ccbFile);
				// Convert, print and store them as the size of the uncompressed data
				fileList[i].uncompressedSize = (uint32_t)(((unsigned char)readBytes[2] << 16) | ((unsigned char)readBytes[1] << 8) | (unsigned char)readBytes[0]);
				cout << "Uncompressed File Size: " << dec << fileList[i].uncompressedSize << " bytes" << endl << endl;
			}
			// If -l was passed at command line, generate a .lst file for use with CCB Constructor
			if (GENERATE_LST) {
				// Open a new list file for writing
				FILE* lstFile = fopen(strcat(argv[1], ".lst"), "w");
				// Write the number of files
				fprintf(lstFile, "%d\n", ((int)sizeof(fileList)/(int)sizeof(fileList[0])));
				// Write the CCB version number
				fprintf(lstFile, "%d\n", version);
				// Write the name, file type, compression type, and uncompressed size of each file
				for (int i = 0; i < sizeof(fileList)/sizeof(fileList[0]); i++) {
					fprintf(lstFile, "%s %d %d %d\n", fileList[i].name.c_str(), (int)fileList[i].fileType, (int)fileList[i].compressionType, fileList[i].uncompressedSize);
				}
			}
			// If -i was passed at command line, do not proceed to extraction.
			if (INFO_ONLY) {
				return 0;
			}
			// If -d was passed at command line, create a new directory and enter it
			if (NEW_DIRECTORY) {
				if (GENERATE_LST) {
					filesystem::create_directory(string(argv[1]).substr(0, string(argv[1]).find(".ccb.lst")));
					chdir(string(argv[1]).substr(0, string(argv[1]).find(".ccb.lst")).c_str());
				} else {
					filesystem::create_directory(string(argv[1]).substr(0, string(argv[1]).find(".ccb")));
					chdir(string(argv[1]).substr(0, string(argv[1]).find(".ccb")).c_str());
				}
			}
			// Decompressing and writing files
			for (int i = 0; i < sizeof(fileList)/sizeof(fileList[0]); i++) {
				// Read in compressed data
				delete readBytes;
				readBytes = new unsigned char[fileList[i].compressedSize];
				fread(readBytes, sizeof(unsigned char), (size_t)fileList[i].compressedSize, ccbFile);
				unsigned char* uncompressedData;
				uncompressedData = new unsigned char[fileList[i].uncompressedSize];
				// If the file is compressed with LZ11, it is written by LZ11Decode().
				if (fileList[i].compressionType == 0x11 || fileList[i].compressionType == 0x40) {
					LZ11Decode(fileList[i].name, readBytes, uncompressedData, fileList[i].compressedSize, fileList[i].uncompressedSize, fileList[i].compressionType);
				} else if (fileList[i].compressionType == 0x10) {
					LZ10Decode(fileList[i].name, readBytes, uncompressedData, fileList[i].compressedSize, fileList[i].uncompressedSize, fileList[i].compressionType);
				} else if (fileList[i].compressionType == 0x30) {
					// If the file is compressed with RLE, it is written by RLEDecode().
					RLEDecode(fileList[i].name, readBytes, uncompressedData, fileList[i].compressedSize, fileList[i].uncompressedSize, fileList[i].compressionType);
				} else {
					// If the file is uncompressed, the data is written here
					cout << "Extracting " << fileList[i].name << endl;
					FILE* outputFile = fopen(fileList[i].name.c_str(), "wb");
					fwrite(readBytes, sizeof(char), fileList[i].compressedSize, outputFile);
				}
			}
		}
	}
	return 0;
}