# dmesg-recent

Output `dmesg` lines timestamped since last invocation.

## Usage

I have this hourly command in my crontab so that any problems with the
disk get emailed to root:

    dmesg | /usr/local/bin/dmesg-recent /run/dmesg-recent.stamp | \
        grep -iE 'reset high-speed|EXT3-fs error|read.only'
