cat /proc/uptime|awk -F. '{run_days=$1 / 86400;run_hour=($1 % 86400)/3600;run_minute=($1 % 3600)/60;printf("%dD:%dH:%dM",run_days,run_hour,run_minute)}'
