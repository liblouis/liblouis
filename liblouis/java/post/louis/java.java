package louis;

import java.io.ByteArrayInputStream;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.Map;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CShortPointer;

import static louis.c.dotMapping;
import louis.c.intCharTupple;
import static louis.c.MAXSTRING;

public final class java {
	private java() {}
	public static void main(String[] args) {}

	@CEntryPoint(name = "lou_utils_showDots")
	public static void lou_utils_showDots(IsolateThread thread,
	                                      CShortPointer dots,
	                                      int dotslen,
	                                      CCharPointer scratchBuf) {
		intCharTupple dotMapping = dotMapping();
		Map<Integer,Character> map = new HashMap<>();
		for (int i = 0;; i++) {
			intCharTupple tuple = dotMapping.addressOf(i);
			int key = tuple.key();
			byte val = tuple.value();
			if (key == 0) break;
			map.put(key, (char)val);
		}
		String s = louis.utils.showDots(read(dots, dotslen), map);
		if (s.length() + 1 > MAXSTRING())
			s = s.substring(0, MAXSTRING() - 1);
		write(scratchBuf, s);
	}

	private static short[] read(CShortPointer from, int len) {
		short[] array = new short[len];
		for (int i = 0; i < len; i++)
			array[i] = from.read(i);
		return array;
	}

	private static void write(CCharPointer to, String string) {
		try {
			ByteArrayInputStream is = new ByteArrayInputStream(string.getBytes("UTF-8"));
			int i = 0;
			int b;
			while ((b = is.read()) != -1)
				to.write(i++, (byte)b);
			to.write(i, (byte)0);
		} catch (UnsupportedEncodingException e) {
			throw new RuntimeException(e);
		}
	}
}
