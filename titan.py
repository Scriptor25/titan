import lldb

def TypeViewSummary(val: lldb.SBValue, _):
    size_value = val.GetNumChildren()
    return f"size={size_value}"

class TypeViewSynthetic:
    def __init__(self, val: lldb.SBValue, _):
        self.val = val

        self.stride: lldb.SBValue
        self.buffer: lldb.SBValue

        self.buffer_impl: lldb.SBValue
        self.buffer_impl_start: lldb.SBValue
        self.buffer_impl_finish: lldb.SBValue

        self.stride_value: int
        self.count_value: int

        self.base_type: lldb.SBType

    def update(self) -> bool:
        self.stride = self.val.GetChildMemberWithName("m_Stride")
        self.buffer = self.val.GetChildMemberWithName("m_Buffer")

        self.buffer_impl = self.buffer.GetChildMemberWithName("_M_impl")
        self.buffer_impl_start = self.buffer_impl.GetChildMemberWithName("_M_start")
        self.buffer_impl_finish = self.buffer_impl.GetChildMemberWithName("_M_finish")

        self.stride_value = self.stride.GetValueAsUnsigned()

        self.count_value = (self.buffer_impl_finish.GetValueAsUnsigned() - self.buffer_impl_start.GetValueAsUnsigned()) // self.stride_value if self.stride_value else 0

        val_type: lldb.SBType = self.val.GetType()
        self.base_type = val_type.GetTemplateArgumentType(1)

        return True

    def has_children(self) -> bool:
        return True

    def num_children(self, max) -> int:
        return self.count_value

    def get_child_at_index(self, index: int) -> lldb.SBValue | None:
        if index < 0 or index >= self.count_value:
            return None
        
        offset = index * self.stride_value
        address = self.buffer_impl_start.GetValueAsUnsigned() + offset

        return self.val.CreateValueFromAddress(f"[{index}]", address, self.base_type)

def __lldb_init_module(debugger: lldb.SBDebugger, _):
    debugger.HandleCommand("type summary add -x '^titan::TypeView<.*>$' -F titan.TypeViewSummary")
    debugger.HandleCommand("type synthetic add -x '^titan::TypeView<.*>$' --python-class titan.TypeViewSynthetic")
