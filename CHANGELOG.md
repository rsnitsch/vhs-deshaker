# Changelog

## v1.1.0

*Release date: 2023/09/24*

* Raw video output to stdout can be enabled by specifying `stdout` as output file. You can use this
  to pipe the raw video data directly to ffmpeg. Thus, you do not have to keep around an intermediate
  `.avi` output file anymore that is extremely large due to the lossless HuffYUV codec.

## v1.0.3

*Release date: 2023/09/21*

* Fixes a crash that occurred when no line starts have been detected for an entire frame (commit 6312847)

## v1.0.2

*Release date: 2023/08/29*

* Only attempt to detect line starts/ends in rows with pure black at the edge. (commit 1199036)
* Fix rare bugs in the line-start gap extrapolation logic (commits e80e373 and b9c1527)
* Improve error messages, make them more informative and user-friendly
* Make parsing of framerate parameter more strict and print the framerate that is being used (and
  where it is taken from, i.e. from commandline parameter or input file).
* Fix calculation of line-start values for other `colRange` values (commmit 0bd2627)
  * Not relevant for users yet, but an important preparation for the commandline options
    that are planned for allowing to change parameters like `colRange`
* refactor line gap interpolation. Interpolation and extrapolation are now performed by
  two separate functions, resulting in much cleaner code (commit bdb7e95).
* The **Docker image** is now based on Debian 12 (bookworm) and uses OpenCV 4.6.0. Previously
  Debian 11 (bullseye) was used with OpenCV 4.5.3.

## v1.0.1

*Release date: 2023/04/09*
 
* Fix linear interpolation of line_start gaps (commit 7fad095)
* Upgrade OpenCV from 4.5.3 to 4.7.0 (the Docker image still uses 4.5)

## v1.0.0

*Release date: 2023/03/17*

Original release.
