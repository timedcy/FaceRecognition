%script: batchRename.m
%To rename and organize training and testing images
clear all
trainRoot = 'C:\Users\Zhang\Desktop\FaceRecognition\image\train\';
matchRoot = 'C:\Users\Zhang\Desktop\FaceRecognition\image\match\';

curNumTags = 60;

for ii = 1:curNumTags
    idxDir = sprintf('%d',ii);
    trainList = dir([trainRoot idxDir '\*.jpg']);
    matchList = dir([matchRoot idxDir '\*.jpg']);
    
    for jj = 1:size(trainList,1)
        trainRename = sprintf('%d_%d.jpg',ii,jj);
        fromName = [trainRoot idxDir '\' trainList(jj).name];
        toName = [trainRoot idxDir '\' trainRename];
        if (~strcmp(fromName, toName))
            movefile(fromName, toName );
        end
    end
    
    for jj = 1:size(matchList,1)
        matchRename = sprintf('%d_%d.jpg',ii,jj);
        matchfromName = [matchRoot idxDir '\' matchList(jj).name];
        matchtoName = [matchRoot idxDir '\' matchRename];
        %movefile([matchRoot idxDir '\' matchList(jj).name], [matchRoot idxDir '\' matchRename]);
        if (~strcmp(matchfromName, matchtoName))
            movefile(matchfromName, matchtoName );
        end
    end
end