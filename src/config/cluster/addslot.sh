start=$1
end=$2
ip=192.168.10.131
port=$3

for slot in `seq ${start} ${end}`
do
    echo "slot:${slot}"
    redis-cli -h ${ip} -p ${port} cluster addslots ${slot}
done

