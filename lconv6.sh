#!/bin/bash

logDir="data/2108"
programName="lconv6"
outFile="sensor-svm.txt"

echo "lconv front-end for ${programName}"
#printf "로그파일이 저장된 디렉토리를 입력하세요: "
#read logDir
printf "출력 파일을 저장할 디렉토리 이름을 입력하세요: "
read outDir

dirCnt=0
# 로그 파일 폴더 순회: 폴더 단위로 실행
for i in ${logDir}/*; do
    logSubDir=$(basename ${i}) #: $i: data/2108/001, data/2108/002, ... 
    echo "[${logSubDir}]" # 000, 001, 002, ...

    # lconv1 출력파일 생성
    cp ${i}/sensor-a?.txt ./
    ./lconv1 sensor-ax.txt
    ./lconv1 sensor-ay.txt
    ./lconv1 sensor-az.txt

    # lconv6 출력파일 생성
    ./${programName}
    destDir="${outDir}/${logSubDir}"
    mkdir -p $destDir
    mv $outFile $destDir
    let dirCnt=$dirCnt+1
    rm sensor-a*.txt
    echo 
done

echo 총 $dirCnt개 디렉토리를 생성했습니다.
