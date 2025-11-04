USD_VERSION=v25.11


mkdir tmp
cd tmp
lll
git clone -b ${USD_VERSION} https://github.com/PixarAnimationStudios/OpenUSD.git

mkdir install
python3 ./OpenUSD/build_scripts/build_usd.py --no-python --no-materialx --usd-imaging --openimageio --embree --build ./build --src src ./install
