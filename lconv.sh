#!/bin/bash

outFile="sensor-out.txt"
winFile="sensor-win.txt"
logDir="data/210614"
subDir=("/left/" "/right/")
winList=(5 10 25)
#outDir="out"

echo "lconv front-end for lconv4"
#printf "로그파일이 저장된 디렉토리를 입력하세요: "
#read logDir
printf "출력 파일을 저장할 디렉토리 이름을 입력하세요: "
read outDir

echo "outDir: ${outDir}"
exit

dirCnt=0
for i in ${subDir[@]}; do # left, right
    for j in ${logDir}${i[@]}*; do # left, right 내부의 개별 디렉토리 접근 (1, 2, 3, 4, 5)
        # echo ${j/${logDir}${i}}
        # echo $j

        # lconv1 출력파일 생성
        cp ${j}/sensor-a?.txt ./
        ./lconv1 sensor-ax.txt
        ./lconv1 sensor-ay.txt
        ./lconv1 sensor-az.txt

        # time-window 별 lconv3 출력파일 생성
        for k in ${winList[@]}; do # (5, 10, 25)
            ./lconv3 $k
            destDir="${outDir}/win${k}$i${j/${logDir}${i}}"
            echo "> "$destDir
            mkdir -p $destDir
            mv $outFile $destDir
            mv $winFile $destDir
            echo
            let dirCnt=$dirCnt+1
        done # end of k
        rm sensor-a*.txt
    done # end of j
    echo
done # end of i

echo 총 $dirCnt개 디렉토리를 생성했습니다.