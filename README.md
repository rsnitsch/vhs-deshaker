# vhs-deshaker

This tool is for fixing a very specific horizontal shaking issue that affects some digitized VHS videos.

Input example:

https://user-images.githubusercontent.com/1746428/225921679-25f9ba88-bee2-4557-9181-5fd0df6a07b4.mp4

vhs-deshaker output:

https://user-images.githubusercontent.com/1746428/225921734-024c3526-5193-4755-940d-00e3f5f86097.mp4

I do not know what causes this type of horizontal shaking. I first encountered it when digitizing some of our old VHS cassettes with family/childhood videos. Some cassettes were more affected than others, suggesting a mechanical/physical problem.

## How does vhs-deshaker work?

Unfortunately I was unable to fix our videos with traditional video deshaking software. So I decided to try and create my own algorithm for fixing this nasty issue.

vhs-deshaker analyses each frame separately. It uses the black area at the left and right borders to determine the horizontal shift for each line. These shifts are then cleaned up, interpolated and smoothed. The interpolation is to fill in gaps because in dark scenes it will be impossible to find the black border for many lines. Finally the lines in the frame can be deshifted, which should remove most of the horizontal shaking.

## Install

You can download the latest binary from the releases page. The accompanying opencv DLL files must be kept in the same folder as the executable.

It is recommended to also install the ffmpeg binaries on your system and edit your PATH environment variable such that they can be executed from the commandline. (ffmpeg is needed to add back the audio stream to the output files, see below.)

## Usage

In general the command can be executed like this:

    vhs-deshaker <input-file> <output-file> [<framerate>]

The framerate parameter is optional. If omitted, the framerate will be the same as in the input file.

Unfortunately vhs-deshaker can only process video streams. The audio will not be included in the output file. Therefore you have to add back the audio stream manually to the output file. Furthermore, you should know that vhs-shaker uses the lossless HuffYUV video codec to generate the output file. Therefore the output files will be huge and you should make sure your disk has enough free space. Also, the output files must have .avi format / extension because mp4 does not support the HuffYUV codec.

I recommend to use ffmpeg to add back the audio stream to the deshaked video file. For example:

    # Remove horizontal shaking
    vhs-deshaker.exe input.avi deshaked.avi

    # Add back the audio stream and recode the video stream from HuffYUV to H.264
    ffmpeg -vn -i input.avi -an -i deshaked.avi -c:a copy final.mp4

For details on how to use ffmpeg to mux audio/video streams you can read this StackOverflow post: https://stackoverflow.com/a/12943003/623685

## Build instructions

See BUILD.md.

## See also

Online threads discussing the issue:
- https://forum.videohelp.com/threads/394670-VHS-Horizontal-Stabilisation
- https://forum.videohelp.com/threads/392186-Way-too-shakey-captured-VHS-video
- https://forum.doom9.org/showthread.php?t=174756
