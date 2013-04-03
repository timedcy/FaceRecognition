%Script file: processFERET.m

%
MATCH_PATH = 'D:/Zhi/colorferet/dvd2/gray_feret_cd1/test/inp/';
MATCH_DEST = 'C:/Users/Zhi/Desktop/temp/match/';
TRAIN_PATH = 'D:/Zhi/colorferet/dvd2/gray_feret_cd2/data/thumbnails/';
TRAIN_DEST = 'C:/Users/Zhi/Desktop/temp/train/';

matchList = dir([MATCH_PATH '*.jpg']);
trainList = dir([TRAIN_PATH '*.jpg']);
matchID = zeros(1,length(matchList));
matchType = cell(1,length(matchList));
trainID = zeros(1, length(trainList));
trainType = cell(1,length(trainList));

for ii = 1:length(matchList)
    matchID(ii) = str2double(matchList(ii).name(1:5));
    matchType{ii} = matchList(ii).name(6:7);
end

for ii = 1:length(trainList)
    trainID(ii) = str2double(trainList(ii).name(1:5));
    trainType{ii} = trainList(ii).name(6:7);
end

newID = zeros(size(matchID));
newID(1) = matchID(1);
curFolder = 1;
for ii = 1:length(matchList)
    curID = matchID(ii);
    if ( newID(curFolder) ~= curID)
        curFolder = curFolder + 1;
        newID(curFolder) = curID;
        %copy match images
%         source = sprintf('%s%s',MATCH_PATH, matchList(ii).name);
%         dest = sprintf('%s%d%s',MATCH_DEST, curFolder,'/');
%         copyfile(source, dest);
        %copy train images
        trainPos = find(trainID == curID);
        for jj = 1:length(trainPos)
            if (strcmp(trainType{trainPos(jj)}, 'fa'))||(strcmp(trainType{trainPos(jj)}, 'fb'))
                if ( strcmp(matchList(ii).name, trainList(trainPos(jj)).name))
                    continue;
                end
                source = sprintf('%s%s',TRAIN_PATH, trainList(trainPos(jj)).name);
                dest = sprintf('%s%d%s',TRAIN_DEST, curFolder,'/');
                copyfile(source,dest);
            end
        end
    else
%         source = sprintf('%s%s',MATCH_PATH, matchList(ii).name);
%         dest = sprintf('%s%d%s',MATCH_DEST, curFolder, '/');
%         copyfile(source,dest);
    end
end