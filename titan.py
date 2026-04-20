import lldb

def TypeViewSummary(val: lldb.SBValue, _):
    size_value = val.GetNumChildren()
    return f"size={size_value}"

class TypeViewSynthetic:
    def __init__(self, val: lldb.SBValue, _):
        self.val = val

    def update(self) -> bool:
        self.chunks = self.val.GetChildMemberWithName("m_Chunks")

        impl = self.chunks.GetChildMemberWithName("_M_impl")
        start = impl.GetChildMemberWithName("_M_start")
        finish = impl.GetChildMemberWithName("_M_finish")

        chunk_type = start.GetType().GetPointeeType()
        chunk_size = chunk_type.GetByteSize()

        self.chunk_count = (finish.GetValueAsUnsigned() - start.GetValueAsUnsigned()) // chunk_size

        self.base_type = self.val.GetType().GetTemplateArgumentType(1)

        self.stride = self.base_type.GetByteSize()
        self.count = 0

        print(self.chunk_count)

        self.chunk_meta = {}

        for i in range(0, self.chunk_count):
            address = start.GetValueAsUnsigned() + i * chunk_size
            chunk = self.val.CreateValueFromAddress("chunk", address, chunk_type)

            data = chunk.GetChildMemberWithName("data")
            count = chunk.GetChildMemberWithName("count")

            data_value = data.GetValueAsAddress()
            count_value = count.GetValueAsUnsigned()

            self.chunk_meta[i] = (data_value, count_value)
            self.count += count_value

        return True

    def has_children(self) -> bool:
        return True

    def num_children(self) -> int:
        return self.count

    def get_child_at_index(self, index: int) -> lldb.SBValue | None:
        if index < 0 or index >= self.count:
            return None
        
        chunk_index = index // 64
        local_index = index % 64

        (data, count) = self.chunk_meta[chunk_index]

        address = data + local_index * self.stride

        return self.val.CreateValueFromAddress(f"[{index}]", address, self.base_type)
        
        return None

def __lldb_init_module(debugger: lldb.SBDebugger, _):
    debugger.HandleCommand("type summary add -x '^titan::detail::TypeView<.*>$' -F titan.TypeViewSummary")
    debugger.HandleCommand("type synthetic add -x '^titan::detail::TypeView<.*>$' --python-class titan.TypeViewSynthetic")
