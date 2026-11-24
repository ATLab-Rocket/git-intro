"""Reviewable source/metadata invariants; does not execute MCU code."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
for chip in ('F407', 'L476'):
    project = root / ('ATlab_CAN_' + chip)
    assert (project / ('ATlab_CAN_' + chip + '.ioc')).is_file()
    ET.parse(project / '.project')
    metadata = ET.parse(project / '.cproject')
    configs = metadata.findall('.//storageModule[@moduleId="cdtBuildSystem"]/configuration')
    assert {cfg.get('name') for cfg in configs} == {'Normal', 'Loopback'}
    for cfg in configs:
        expected = 'ATLAB_CAN_TEST_LOOPBACK=' + ('1' if cfg.get('name') == 'Loopback' else '0')
        assert any(e.get('value') == expected for e in cfg.findall('.//listOptionValue'))
        for source in cfg.findall('./sourceEntries/entry'):
            assert (project / source.get('name')).is_dir()
    for folder in ('protocol', 'firmware'):
        for source in (root / folder).glob('*.[ch]'):
            dest = project / 'Core' / ('Inc' if source.suffix == '.h' else 'Src') / source.name
            assert source.read_bytes() == dest.read_bytes(), f'Outdated copy: {dest}'
    assert list((project / 'Core/Startup').glob('*.s'))
    assert list(project.glob('*_FLASH.ld')) and list(project.glob('*_RAM.ld'))
    msp = next((project / 'Core/Src').glob('*hal_msp.c')).read_text()
    assert 'GPIO_AF9_CAN1' in msp and 'GPIO_PIN_8|GPIO_PIN_9' in msp
    assert 'ATLAB_Demo_Run();' in (project / 'Core/Src/main.c').read_text()
print('PASS: standalone metadata, two mode definitions, synchronized sources, startup/linkers, AF9 pins')
