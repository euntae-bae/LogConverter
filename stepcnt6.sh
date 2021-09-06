#!/bin/bash
readFile="sensor-svm.txt"
outFile="sensor-step.txt"

echo "stepcnt front-end"
printf "SVM 파일이 저장된 디렉토리를 입력하세요: "
read readDir

if [ ! -e $readDir ]; then
    echo "E: $readDir은(는) 존재하지 않는 디렉토리입니다."
    exit -1
fi

fileCnt=0
printf "실험회차\t걸음수\n" >> $outFile
for i in ${readDir}/*; do
    if [ -d $i ]; then
        echo $i
        cp $i/$readFile ./
    else
        continue
    fi
    printf "$(basename $i)\t" >> $outFile
    ./stepcnt $(basename $i) $readFile >> $outFile
    echo >> $outFile
    let fileCnt=$fileCnt+1
done

rm $readFile
mv $outFile $readDir/
echo "총 $fileCnt개 파일에 대한 걸음수 분석이 완료됐습니다."
