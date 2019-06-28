package louis;

import java.util.Collections;
import java.util.List;

import org.graalvm.nativeimage.c.CContext;
import org.graalvm.nativeimage.c.constant.CConstant;
import org.graalvm.nativeimage.c.function.CFunction;
import org.graalvm.nativeimage.c.struct.CField;
import org.graalvm.nativeimage.c.struct.CStruct;
import org.graalvm.word.PointerBase;

@CContext(c.Headers.class)
public final class c {
	private c() {}

	@CConstant
	public static native int MAXSTRING();

	@CFunction(value = "var_dotMapping")
	public static native intCharTupple dotMapping();

	@CStruct
	public interface intCharTupple extends PointerBase {
		intCharTupple addressOf(int index);
		@CField int key();
		@CField void key(int key);
		@CField byte value();
		@CField void value(byte value);
	}

	static class Headers implements CContext.Directives {
		@Override
		public List<String> getHeaderFiles() {
			return Collections.singletonList("\"" + System.getProperty("internal.h.path") + "\"");
		}
	}
}
