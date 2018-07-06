uptime|awk '{print $3" " $5}'|awk '{printf "%sD:%s>", $1, $2}'|awk -F"," '{print $1}'|awk -F":" '{printf "%s:%sH:%sM\n", $1, $2, $3}'
