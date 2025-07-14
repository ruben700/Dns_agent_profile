# agent_functions/whoami.py

from mythic_container.MythicCommandBase import (
    CommandBase,
    TaskArguments,
    CommandAttributes,
    SupportedOS,
    MythicTask,
    AgentResponse,
)

class WhoamiArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = []  # sin parámetros

    async def parse_arguments(self):
        return

class WhoamiCommand(CommandBase):
    cmd              = "whoami"
    needs_admin      = False
    help_cmd         = "whoami"
    description      = "Muestra el usuario actual"
    version          = 1
    author           = "Ruben"
    argument_class   = WhoamiArguments
    attackmapping    = ["T1087"]
    supported_ui_features = ["callback_table:whoami"]
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: WhoamiArguments) -> WhoamiArguments:
        task.display_params = ""
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
