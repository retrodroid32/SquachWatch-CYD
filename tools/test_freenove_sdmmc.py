from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/"src/sd_log.cpp").read_text()
m=(root/"src/main.cpp").read_text()
b=(root/"src/ui_sd_browser.cpp").read_text()
h=(root/"include/sd_card_backend.h").read_text()
assert "#define SQW_CARD SD_MMC" in h and "#define SQW_CARD SD" in h
assert "SD_MMC.setPins(5,4,6,7,2,3)" in s
assert 'SD_MMC.begin("/sd",false,false,' in s
assert 'SD_MMC.begin("/sd",true,false,' in s
assert "MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT" in s
worker=s.split("void SdLog::monitorEntry(void* ctx) {",1)[1].split("#else",1)[0]
assert "SD_MMC.end()" in worker and "self->begin()" in worker
assert "SPI.begin(" not in worker and "SD.readRAW(" not in worker
assert "SQW_CARD.open(_filename, FILE_APPEND)" in s
assert "SQW_CARD.open(folder)" in b and "SQW_CARD.remove(chosen)" in b
assert m.index('freenoveBootProgress("Starting SDMMC worker...")') > m.index("MeshTalk::begin();")
print("Freenove SDMMC pin, backend and worker guards PASS")
