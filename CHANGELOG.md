# Changelog

## v1.0.2

*Release date: 2023/08/29*

* Only attempt to detect line starts/ends in rows with pure black at the edge. (commit 1199036)
* Fix rare bugs in the line-start gap extrapolation logic (commits e80e373 and b9c1527)
* Improve error messages, make them more informative and user-friendly
* Make parsing of framerate parameter more strict
* Fix calculation of line-start values for other `colRange` values (commmit 0bd2627)
  * Not relevant for users yet, but an important preparation for the commandline options
    that are planned for allowing to change parameters like `colRange`
* refactor line gap interpolation. Interpolation and extrapolation are now performed by
  two separate functions, resulting in much cleaner code (commit bdb7e95).

## v1.0.1

*Release date: 2023/04/09*
 
* Fix linear interpolation of line_start gaps (commit 7fad095)
* Upgrade OpenCV from 4.5.3 to 4.7.0 (the Docker image still uses 4.5)

## v1.0.0

*Release date: 2023/03/17*

Original release.
