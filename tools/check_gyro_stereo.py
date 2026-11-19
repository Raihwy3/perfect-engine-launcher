from pathlib import Path
import subprocess,json,shlex,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
args=shlex.split(row['command']);args=args[:args.index('-o')];args+=['-I'+str(out/'src/generated/ntsc-final')]
subprocess.run([*args,'-UNDEBUG','-I'+str(root/'include'),'-fsanitize=address,undefined',root/'tests/gyro_host.c','-lm','-o',out/'gyro-check'],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([out/'gyro-check'],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
subprocess.run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'vendor/eog-pica/include'),root/'tests/stereo_depth.cpp','-o',out/'stereo-check'],check=True)
subprocess.run([out/'stereo-check'],check=True)

subprocess.run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),root/'tests/motion_sensor.cpp',root/'src/native_motion_sensor.cpp','-o',out/'motion-sensor-check'],check=True)
subprocess.run([out/'motion-sensor-check'],check=True)
subprocess.run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'vendor/eog-pica/include'),root/'tests/dynamic_stereo.cpp','-o',out/'dynamic-stereo-check'],check=True)
subprocess.run([out/'dynamic-stereo-check'],check=True)

subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),root/'tests/flick_stick.c','-lm','-o',out/'flick-check'],check=True)
subprocess.run([out/'flick-check'],check=True)

subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),root/'tests/crosshair_host.c','-o',out/'crosshair-check'],check=True)
subprocess.run([out/'crosshair-check'],check=True)

subprocess.run(["g++","-std=c++20","-O2","-Wall","-Wextra","-Werror","-I"+str(root/"include"),root/"tests/crosshair_sdf_host.cpp","-o",out/"crosshair-sdf-check"],check=True)
subprocess.run([out/"crosshair-sdf-check"],check=True)
