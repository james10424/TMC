wild() {
    # Wild at the Greek by the Go-Go's
    # https://www.youtube.com/watch?v=z0ZsGeNpePA
    # argument 1: index of the song
    # `wild 1` plays "Head Over Heels"

    if [ -z $1 ]; then
        cat <<EOF
1 - Head Over Heels
2 - Our Lips Are Sealed
3 - Forget That Day
4 - Turn To You
5 - Your Thought
6 - Yes Or No
7 - I'm With You
8 - This Town
9 - Can't Stop The World
10 - Vacation
11 - I'm The Only One
EOF
        return 1
    fi
    arr[1]=64
    arr[2]=329
    arr[3]=549
    arr[4]=996
    arr[5]=1433
    arr[6]=1701
    arr[7]=1939
    arr[8]=2171
    arr[9]=2379
    arr[10]=2640
    arr[11]=2808
    osascript ./itunes.applescript 1 ${arr[$1]}
}
wild 1