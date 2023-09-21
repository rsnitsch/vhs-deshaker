# TODOs

## Features

- piping to ffmpeg
  - https://stackoverflow.com/a/61281547/623685
- add `--col-range` commandline option
- add `--framerate` commandline option (replacing the previously used *positional* framerate parameter)
- add commandline parameter to set `TARGET_LINE_START`. This is the regular/ideal width of the pure black
  area at the left and right sides of the video frames. So it could get a new name like `PURE_BLACK_WIDTH`
  and get a corresponding commandline option `--pure-black-width`.
- add `--min-segment-length` commandline option (parameter for `denoise_line_starts`)
- `draw_line_starts` should use a better visualization for negative line_starts. Idea: For a row with negative line_start value, fill up the pixels from the *opposite* side with a bright (e.g. red) color that
  indicates the area where information is lost. Actually this idea should be expanded to all **extreme** line_start values. (big positive line_starts are also leading to data loss.)

## Refactoring / Code cleanup

- `line_ends` do not actually refer to end positions, instead they're just line_starts
  determined by scanning from the end (right side) of the rows. this distinction should be made clearer
  because I bet it will utterly confuse some people.
- `draw_line_starts` adds to the confusion because you can pass `line_ends` with an `x_offset` to draw
  the edge at the right side of the image frame
