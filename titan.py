import lldb

def join_args(*args) -> str:
    return ", ".join(str(arg) for arg in args)

def call(val: lldb.SBValue, label: str, name: str, *args) -> lldb.SBValue:
    return val.CreateValueFromExpression(label, f"(({val.GetTypeName()} *){val.GetAddress().GetLoadAddress(val.GetTarget())})->{name}({join_args(*args)})")

def TypeViewSummary(val: lldb.SBValue, _) -> str:
    size = val.GetNumChildren()
    return f"size={size}"

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

def ArchetypeSummary(val: lldb.SBValue, _) -> str:
    size = val.GetNumChildren()
    return f"size={size}"

class ArchetypeSynthetic:
    def __init__(self, val: lldb.SBValue, _):
        self.val = val

    def update(self) -> bool:
        count_value = call(self.val, "count", "size")
        self.count = count_value.GetValueAsUnsigned()
        return True

    def has_children(self) -> bool:
        return True

    def num_children(self) -> int:
        return self.count

    def get_child_at_index(self, index: int) -> lldb.SBValue | None:
        if index < 0 or index >= self.count:
            return None
        return call(self.val, f"[{index}]", "entry", index)

def ArchetypeEntrySummary(val: lldb.SBValue, _) -> str:
    return val\
        .GetNonSyntheticValue()\
        .GetChildMemberWithName("info")\
        .GetChildMemberWithName("name")\
        .GetSummary()\
        .strip('"')

class ArchetypeEntrySynthetic:
    def __init__(self, val: lldb.SBValue, _):
        self.val = val

    def update(self) -> bool:
        info_value = self.val.GetChildMemberWithName("info")
        name_value = info_value.GetChildMemberWithName("name")
        name = name_value.GetSummary().strip('"')

        pointer_value = self.val.GetChildMemberWithName("pointer")
        pointer = pointer_value.GetValueAsAddress()

        self.value = self.val.CreateValueFromExpression("value", f"(const {name} *){pointer}")
        return True

    def num_children(self) -> int:
        return 1

    def get_child_index(self, name: str) -> int:
        if name == "value":
            return 0
        return -1

    def get_child_at_index(self, index: int) -> lldb.SBValue | None:
        if index == 0:
            return self.value
        return None

def __lldb_init_module(debugger: lldb.SBDebugger, _):
    debugger.HandleCommand("type synthetic add -x '^titan::detail::TypeView<.*>$' --python-class titan.TypeViewSynthetic")
    debugger.HandleCommand("type summary add -x '^titan::detail::TypeView<.*>$' -F titan.TypeViewSummary")

    debugger.HandleCommand("type synthetic add titan::detail::Archetype --python-class titan.ArchetypeSynthetic")
    debugger.HandleCommand("type summary add titan::detail::Archetype -F titan.ArchetypeSummary")

    debugger.HandleCommand("type synthetic add titan::detail::Archetype::ComponentEntry --python-class titan.ArchetypeEntrySynthetic")
    debugger.HandleCommand("type summary add titan::detail::Archetype::ComponentEntry -F titan.ArchetypeEntrySummary")
