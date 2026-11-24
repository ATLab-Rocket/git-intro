"""Copy canonical application sources into both standalone CubeIDE projects."""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[1]
for name in ('ATlab_CAN_F407', 'ATlab_CAN_L476'):
    for folder in ('protocol', 'firmware'):
        for source in (root / folder).glob('*.[ch]'):
            target = root / name / 'Core' / ('Inc' if source.suffix == '.h' else 'Src') / source.name
            shutil.copyfile(source, target)
print('Synchronized both standalone projects')
