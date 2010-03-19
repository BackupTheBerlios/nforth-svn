#!/bin/awk -f
# This script will try to automatically flash an ELF file into a MCU using avrdude.
# The script tries to be smart, so it will flash every ELF section into the
# respective MCU memory area. Also it will verify every section if it is different
# from what has to be flashed, and if it is exactly same it won't flash the MCU
# to avoid wearing out the flash memory.
#
# Command-line options:
# $1 - the ELF file to flash into the MCU
# $2 - additional avrdude parameters (must include -c and possibly -P)

function cmd(_) {
    _ | getline
    close(_)
    return $0
}

function maketemp(suffix) {
    return cmd("mktemp /tmp/tmp"suffix".XXXXXX")
}

function flash(mem, sect_sw, start, len) {
    tmpf = maketemp("."mem)
    TMPFILES[tmpf] = 1
    system(OBJCOPY" -O binary "sect_sw" "FN" "tmpf)
    size = strtonum(cmd("stat -c %s "tmpf))

    # Avoid flashing unchanged sections, so we'll first invoke
    # avrdude in verify mode and flash the section only if it reports mismatch
    printf ("Verifying section ."mem" ...")
    if (start == 0 && len == size) {
        # will use the whole file
    } else if (start < size && start + len <= size) {
        tmpf2 = maketemp("."mem)
        TMPFILES[tmpf2] = 1
        system("dd if="tmpf" of="tmpf2" bs=1 count="len" skip="start" >/dev/null 2>&1")
        tmpf = tmpf2
    }

    if (OPT_FORCE || (system (DUDECMD" -U "mem":v:"tmpf":r >/dev/null 2>&1") != 0)) {
        DUDEARG = DUDEARG" -U "mem":w:"tmpf":r"
        WORK_TO_DO = 1
        printf ("  will flash\n")
    } else
        printf (" no change\n")
}

function do_help() {
    print "Usage: flash-elf.awk [elf-file-name] {options}"
    print " -D \"...\"   Additional command-line options to pass to avrdude"
    print " -X memtype Type of AVR memory not to flash (e.g. eeprom, fuse, flash)"
    print " -f         Force flashing, even if content check shows no change"
    print " -h         Display this short usage summary"
    exit
}

BEGIN {
    AVRDUDE = "avrdude"
    OBJDUMP = "avr-objdump"
    OBJCOPY = "avr-objcopy"

    # Handle command-line arguments
    OPT_FORCE = 0
    for (i = 1; i < length(ARGV); i++)
        if (substr (ARGV [i], 1, 1) == "-") {
            sw = substr (ARGV [i], 2);
            if (sw == "D")
                DUDEOPTS = ARGV[++i];
            else if (sw == "X")
                EXCLUDE_MEM [ARGV [++i]] = 1
            else if (sw == "f")
                OPT_FORCE = 1
            else if (sw == "h")
                do_help()
            else {
                print "Unknown command-line option `"ARGV [i]"', try -h"
                exit
            }
        } else
            FN = ARGV[1]

    if (FN == "")
        do_help()

    # Basic command line: disable safe-mode because we'll program the fuses
    # if the .fuses section is present, and won't touch it if it's not present;
    # in any case having fuses specified in the .c file is much less error-prone
    # than guessing the hex value at the command line.
    DUDECMD = AVRDUDE" "DUDEOPTS" -u"
    DUDEARG = ""
    WORK_TO_DO = 0

    foundmem = 0
    _=DUDECMD" -v -q -n 2>&1"
    while ((_ | getline) > 0) {
        if ($1 ~ "-+") {
            foundmem = 1
            continue
        } else if ($0 == "")
            foundmem = 0

        if (foundmem == 0)
            continue

        avrsize[$1] = $7
    }
    if (close(_) != 0) {
        print "Could not determine MCU memory types & sizes - check connections"
        exit
    }

    _=OBJDUMP" -h "FN
    while ((_ | getline) > 0) {
        if ( $0 !~ "^ +[0-9]+ +")
            continue
        elfsize[$2]=strtonum("0x"$3)
    }
    close(_)

    for (mem in avrsize) {
        start = 0
        count = 0
        delete sect

        if (mem in EXCLUDE_MEM)
            continue;

        if (mem == "flash")
        {
            sect [0] = ".text"
            sect [1] = ".data"
        }
        else if (mem == "lfuse") {
            sect [0] = ".fuse"
            start = 0
            count = 1
        } else if (mem == "hfuse") {
            sect [0] = ".fuse"
            start = 1
            count = 1
        } else if (mem == "efuse") {
            sect [0] = ".fuse"
            start = 2
            count = 1
        } else
            sect [0] = "."mem

        sect_sw = ""
        sect_size = 0
        for (x = 0; x < length (sect); x++)
            if (sect [x] in elfsize)
            {
                sect_sw = sect_sw" -j "sect [x];
                sect_size += elfsize [sect [x]]
            }

        if (count == 0)
            count = sect_size

        if (sect_size)
            flash(mem, sect_sw, start, count)
    }

    if (WORK_TO_DO)
    {
        # Flash everything, if we have what to flash
        if (DUDEARG != "")
            system (DUDECMD""DUDEARG)

        # Clean up temporary files
        for (x in TMPFILES)
            system("rm -f "x)
    }

    exit
}
