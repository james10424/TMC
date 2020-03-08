# argument1: the index of the song in the current playlist starting at 1
# argument2: the timestamp in seconds into the song
on run argv
    tell application "iTunes"
        stop
    end tell
    tell application "System Events"
        tell process "iTunes"
            click UI element 1 of row (item 1 of argv as number) of table 1 of scroll area 2 of splitter group 1 of window "iTunes"
        end tell
    end tell
    tell application "iTunes"
        set player position to (item 2 of argv as number)
        play
    end tell
end run