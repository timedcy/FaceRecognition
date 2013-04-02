%calculate feature distance matrix
FEATURE_LEN = 5120;
RATIO = 5;

numIntra = 0;
for ii = 1 : NUM
    for jj = ii+1:NUM
        if ( id(ii) == id(jj))
            %intra
            numIntra = numIntra + 1;
        end
    end
end

l = 0;
for ii = 1:NUM-1
    l = l + ii;
end
numTotal = numIntra*(RATIO+1);
fd = zeros(numTotal, FEATURE_LEN);
label = zeros(numTotal,1);

cnt = 1;
interRatio = ceil(l / numIntra/RATIO);
interCnt = 0;
for ii = 1 : NUM
    for jj = ii+1:NUM
        
        if ( id(ii) == id(jj))
            %intra
            label(cnt) = 1;
            fd(cnt,:) = abs(histLBP(ii,:) - histLBP(jj,:));
        else
            interCnt = interCnt + 1;
            if ( mod(interCnt, interRatio) ~=0)
                continue;
            end
            label(cnt) = 0;
            fd(cnt,:) = abs(histLBP(ii,:) - histLBP(jj,:));
        end
        cnt = cnt+1;
    end
end

fd(cnt:end,:) = [];
label(cnt:end,:) = [];