#!/bin/bash

# NOTE: input file must be aligned by ${BYTES_PER_READ} bytes

# output chunk size
readonly BYTES_PER_LINE=8
#input buffer size
readonly BYTES_PER_READ=64
readonly LINES_IN_BUFFER=$BYTES_PER_READ/$BYTES_PER_LINE

readonly FORMAT_STR_PREFIX=$(printf '%%s:%02X%%04X00' $BYTES_PER_LINE)

printf "" >$2
outaddr=0
wcstr=$(wc -c $1)
fsize=${wcstr% *}
for (( addr = 0 ; addr < $fsize ; addr += $BYTES_PER_READ )) ; do
    bytes=( $(od -An -t u1 -N $BYTES_PER_READ --width=1000 -j $addr $1) )
    out_str=""
    ii=0
    for (( n = 0 ; n < $LINES_IN_BUFFER ; n++ )) ; do
        printf -v out_str $FORMAT_STR_PREFIX "$out_str" $outaddr
        cs=$[$outaddr/256+$outaddr%256]
        line_bytes=""
        for (( i = $ii+$BYTES_PER_LINE-1 ; i >= $ii ; i--)) ; do
            val=${bytes[$i]}
            printf -v line_bytes '%s%02X' "$line_bytes" $val
            cs=$[$cs+$val]
        done
        cs=$[$cs+$BYTES_PER_LINE]
        printf -v out_str '%s%s%02X\n' "$out_str" "$line_bytes" $[(0x100-$cs%256)%256]
        outaddr=$[$outaddr+1]
        ii=$[$ii+$BYTES_PER_LINE]
    done
    printf '%s' "$out_str" >>$2
done
printf ":00000001FF\n" >>$2
