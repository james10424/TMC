Control iTunes from terminal

You don't want to grab the progress bar back and forth to find the song you want to listen in a compilation/live concert, this script is the solution.
This script plays music (video) in iTunes that is very long at an offset (timestamp). I find it very useful when you want to skip to a certain song in a live concert, so you don't have to eyeball the timeline.

Usage:
`osascript ./itunes.applescript [index] [offset]` plays the `[index]`th song in your currently selected playlist at `[offset]` seconds offset. Note that you need to give the script permission to control iTunes.

Obviously you don't want to remember all the timestamps yourself, checkout `example.sh` to see how I leverage this script

Issue with iTunes:
If you play the same video (I like to play video on iTunes) consecutively (on repeat), the video will go blank but the sound still plays, this is the design of iTunes and there is no way to turn it off. However, if you double-click the video, it will play normally without going blank, I am exploiting that property with applescript's ability to manipulate UI.

This script is tested to work on iTunes version 12 (Mojave)