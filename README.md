# CCBUtils

CCBUtils is a pair of utilities that handle the proprietary CyberConnect2 Batch (CCB) archives found within Solatorobo: Red the Hunter.
For the first time, these utilities make it possible to not only extract these files, but reconstruct them with modifications.

## What do these tools do?

CCB Extractor can extract every file from every CCB file in the game* and decompress it appropriately. Correct extraction and decompression
was verified by comparing the extracted files with those extracted by Console Tool, which was the only existing tool that could properly work
with these archives. The output is identical.

CCB Constructor can compress files appropriately and create CCB archives that can be re-inserted into the game's ROM. Even when modified, these files are seen as valid
by the game engine and are 100% compatible with the game's code.

## What do these tools NOT do?

CCB Extractor does not currently have the option to leave extracted files in their compressed state, it will always decompress them before writing.

CCB Constructor does not produce output CCB files that are perfectly byte-matching with the originals, even when no modifications have been made to them.
This is due to differences in the exact compression algorithms used by me for this project and those used by the game's developers. This likely cannot be
fixed without trial and error, and will <i>not</i> be a priority to fix, as the compressed data is still valid and decompresses perfectly.

Neither of these tools allow you to extract files from or re-insert files within a DS ROM. You must use an external tool for that.

## What questions are still to be answered?

No tests have been done yet on a ROM without an anti-piracy patch, and there does not seem to be much available information on how exactly the game's
anti-piracy check works.

I made the bold claim earlier that CCB Extractor and CCB Constructor work perfectly with every CCB archive in the game. This has not been tested fully,
and it is not currently known if there are any CCB files that differ in some way from the expected format.

## Build instructions

This project was designed to be built with MinGW for Cygwin for Windows binaries, or Linux using the g++ compiler. However, it will likely compile under any
C++ compiler.

  ### Cygwin instructions

  ```
  x86_64-w64-mingw32-g++ --static -o CCB[UTIL_NAME].exe CCB[UTIL_NAME].cpp
  ```

  ### Linux instructions

  ```
  g++ --static -o CCB[UTIL_NAME] CCB[UTIL_NAME].cpp
  ```

## Special Thanks

I would like to take a moment to thank @PeterLemon for taking the time to put together his <a href="https://github.com/PeterLemon/Nintendo_DS_Compressors">Nintendo_DS_Compressors</a>
repository. I can take absolutely no credit for the LZ11 and RLE compression and decompression functions used in these utilities as they are almost exactly the same as the original
code written by @PeterLemon. Using this C code saved me so much time, and allowed me to focus on my goal of actually documenting and modding this game rather than trying to wrap
my thick skull around binary compression algorithms.
