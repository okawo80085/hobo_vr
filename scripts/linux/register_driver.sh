find ~ -iname vrpathreg

vr_reg_loc=$(find ~ -iname vrpathreg)
hobo_dir="../../hobovr"
true_hobo_dir="$(cd "$(dirname "$hobo_dir")"; pwd)/$(basename "$hobo_dir")"
LD_LIBRARY_PATH="$(dirname "${vr_reg_loc}")" $vr_reg_loc adddriver $true_hobo_dir
$vr_reg_loc show