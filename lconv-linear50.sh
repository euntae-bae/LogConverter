#!/bin/bash

flistFile="result-list.txt"
outFile="sensor-out.txt"
winFile="sensor-win.txt"
logDir="data/210812"
programName="lconv4"

echo "lconv-linear front-end for lconv4c"
#printf "로그파일이 저장된 디렉토리를 입력하세요: "
#read logDir
printf "출력 파일을 저장할 디렉토리 이름을 입력하세요: "
read outDir

mkdir ${outDir}
echo ${outDir}
# 소요시간, 실제 속도, 측정 속도, 오차율
echo -e "소요시간\t실제 속도\t측정 속도\t오차율" >> ${flistFile}

dirCnt=0
# 로그 파일 폴더 순회: 폴더 단위로 실행
for i in ${logDir}/*; do
    logSubDir=$(basename ${i})
    # lconv1 출력파일 생성
    cp ${i}/sensor-a?.txt ./
    ./lconv1 sensor-ax.txt
    ./lconv1 sensor-ay.txt
    ./lconv1 sensor-az.txt

    # 윈도우 사이즈별 lconv4 출력파일 생성
    destDir="${outDir}/${logSubDir}"
    # echo ${destDir} >> ${flistFile}
    ./${programName} 50
    echo "> ${destDir}"
    mkdir -p ${destDir}
    mv $outFile $destDir
    mv $winFile $destDir
    let dirCnt=$dirCnt+1
    rm sensor-a*.txt
    echo
done

mv ${flistFile} ./${outDir}
echo 총 $dirCnt개 디렉토리를 생성했습니다.
