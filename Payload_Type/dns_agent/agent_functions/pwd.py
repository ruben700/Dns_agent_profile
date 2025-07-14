# agent_functions/pwd.py

from mythic_container.MythicCommandBase import (
    CommandBase,
    TaskArguments,
    CommandAttributes,
    SupportedOS,
    MythicTask,
    AgentResponse,
)

class PwdArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        # No parameters
        self.args = []

    async def parse_arguments(self):
        # no hay argumentos
        return

class PwdCommand(CommandBase):
    cmd              = "pwd"
    needs_admin      = False
    help_cmd         = "pwd"
    description      = "Muestra el directorio de trabajo actual"
    version          = 1
    author           = "Ruben"
    argument_class   = PwdArguments
    attackmapping    = ["T1083"]
    supported_ui_features = ["callback_table:pwd"]  
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: PwdArguments) -> PwdArguments:
        task.display_params = ""
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
