import pandas as pd
from Adafruit_IO import Client
from datetime import datetime

now = datetime.now()
time_str = now.strftime("<%d/%m/%Y %H:%M:%S>")

aio = Client('Archit149', '2619ba57dee340489754ca6dac5de74b')

#Data File
file_path = "H:\Project\Data_archive2.csv"
file_data = pd.read_csv(file_path)

#Log File
log_path = "H:\Project\Log.txt"
log_stream = open(log_path, mode = "a")

timestamp = list(file_data['timestamp'])
if timestamp != []:
    t = int(timestamp[-1])
else:
    t = 0       # Accounting for a File that does not have anything Written

timestamp = []
power = []

data = aio.data('energymonitor.tv-po-ts')
data = data[::-1]

d = {}

for i in data:
    m = str(i)
    ind = m.index("value")
    m = m[ind+7:ind+27]
    ts, po = m.split(',')
    po = float(po)
    ts = int(ts)
    if t < ts:
        power.append(po)
        timestamp.append(ts)

d['timestamp'] = timestamp
d['power'] = power

write_data = pd.DataFrame(d)

print(write_data)

pd.DataFrame.to_csv(write_data, file_path, mode = 'a', header = False, index = False, chunksize = 10000)

file_data = pd.read_csv(file_path)
new_t = list(file_data['timestamp'])
new_t = new_t[-1]

if new_t != t:
    log_stream.write(f"\n{time_str} {len(write_data)} rows to file: \"{file_path}\" latest Timestamp: {timestamp[-1]}")
#    print(f"\nSucessfully Wrote {len(write_data)} rows to file upto Timestamp: {timestamp[-1]}")
elif timestamp == []:
    log_stream.write(f"\n{time_str} Nothing to Write to file")
#    print("\nNothing to Write to file")
else:
    log_stream.write(f"\n {time_str} Failed to Write Data to file")
#    print("Failed to Write Data to File")

log_stream.close()
