# BashNUL

escape and de-escape NUL characters.

## Motivation

`bash` ignores `NUL` characters on input, such that `read -N1 </dev/zero` does not terminate in `bash`.

This makes it difficult to skip exactly N bytes when reading from a pipe.

`bashnul` is shall fix that problem with only a little pain.

Instead of `producer | bashscript | consumer` we can write `producer | bashnul | bashscript | bashnul - | consumer`

Note that if you do not use `LC_ALL=C` then `bashnul` will bail out for safety.  So do

Instead of `producer | bashscript | consumer` we can write `producer | LC_ALL=C bashnul -e | LC_ALL=C bashscript | LC_ALL=C bashnul -d | consumer`


## Usage

	git clone https://github.com/hilbix/bashnul.git
	cd bashnul
	make
	sudo make install

What it does:

- on input (`-e`):
  - each `NUL` byte (`00`) will be escaped to `SOH STX` (`01 02`)
  - each `SOH` byte (`01`) will be escaped to `SOH ETX` (`01 03`)
- on output (`-d`):
  - each `SOH STX` combination (`01 02`) will be deescaped to `NUL` (`00`)
  - each `SOH ETX` combination (`01 03`) will be escaped to `NUL` (`01`)

To read exactly `N` bytes within `bash` you need to read the additional `01` bytes as well:

	: readbytes N variable
	readbytes()
	{
	local -n ___var="${2:-REPLY}"
	local ___esc ___tmp
	LC_ALL=C read -rN "$1" ___var || return		# short read
	___esc="$___var"
	while	___esc="${___esc//[^$'\x01']/}"
		___tmp="${#___esc}"
		[ 0 -lt "$___tmp" ]
	do
		___esc=
		LC_ALL=C read -rN "$___tmp" ___esc
		___tmp=$?
		___var="$___var$___esc"
		[ 0 = $___tmp ] || return $___tmp	# short read
	done
	return 0
	}

If you are sure to process the additional `01` bytes with their following bytes properly,
then the (filtered) output can be processed unharmed.


## FAQ

Why not use backslash escapes?

- Because this does not help. `\x00` still cannot be read by `bash`, and mutating it elsewhere is nuts.

Why not UTF-8?

- My `bash` here does read `c0 80` as 2 characters, so this mapping cannot be used
- Hence when Unicode is properly treated, there is no free room to map `NUL` somewhere else
- Usually streams (like `git-fast-export`, HTTP chunked encoding or websockets) operate on bytes, not `UTF-8`.

`LC_ALL=C`?

- That's for safety, because `read -N` behaves stangely if it does not read bytes
- I do not know how to find out if the current locale may use multibyte characters.
  - If you can enlighten me, please state.
- In future, perhaps I can support what `bash` thinks what a character is
  - Without documentation on what a character is for `bash`, it is difficult to support properly
  - see <https://stackoverflow.com/q/67534890/490291>

License?

- Free as free beer, free speech and free baby

