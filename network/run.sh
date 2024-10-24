for data in $(seq 0 26);
do
    for i in $(seq 1 100);
    do
        srun -N 1 -n 2 ./memcpy $((2 ** $data))
    done
done
